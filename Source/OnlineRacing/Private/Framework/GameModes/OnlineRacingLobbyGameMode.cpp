// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/GameModes/OnlineRacingLobbyGameMode.h"

#include "OnlineRacing.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

#include "Framework/GameStates/OnlineRacingLobbyGameState.h"
#include "Framework/PlayerStates/OnlineRacingPlayerState.h"

AOnlineRacingLobbyGameMode::AOnlineRacingLobbyGameMode()
{
	bDelayedStart = true;
	bStartPlayersAsSpectators = true;
	bUseSeamlessTravel = true;
	DefaultPawnClass = nullptr;
	GameStateClass = AOnlineRacingLobbyGameState::StaticClass();
	PlayerStateClass = AOnlineRacingPlayerState::StaticClass();
}

void AOnlineRacingLobbyGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	Super::HandleStartingNewPlayer_Implementation(NewPlayer);

	if (!IsValid(NewPlayer))
	{
		return;
	}

	AOnlineRacingPlayerState* OnlineRacingPlayerState = NewPlayer->GetPlayerState<AOnlineRacingPlayerState>();
	if (!IsValid(OnlineRacingPlayerState))
	{
		UE_LOG(LogOnlineRacing, Warning, TEXT("[Server][LobbyGameMode] Player initialization failed: PlayerState unavailable. Controller=%s"), *GetNameSafe(NewPlayer));
		return;
	}

	OnlineRacingPlayerState->SetLobbyReady(false);
	AssignHostIfNeeded(OnlineRacingPlayerState, NewPlayer->IsLocalController());
	EvaluateLobbyState();
	FString HostState = TEXT("false");
	if (OnlineRacingPlayerState->IsLobbyHost())
	{
		HostState = TEXT("true");
	}

	UE_LOG(LogOnlineRacing, Log,
		TEXT("[Server][LobbyGameMode] Player entered lobby. Player=%s Host=%s"),
		*GetNameSafe(OnlineRacingPlayerState),
		*HostState);
}

void AOnlineRacingLobbyGameMode::Logout(AController* Exiting)
{
	AOnlineRacingPlayerState* ExitingPlayerState = nullptr;
	if (IsValid(Exiting))
	{
		ExitingPlayerState = Exiting->GetPlayerState<AOnlineRacingPlayerState>();
	}

	const bool bHostLeft = IsValid(ExitingPlayerState) && ExitingPlayerState->IsLobbyHost();
	if (bHostLeft)
	{
		ExitingPlayerState->SetLobbyHost(false);
	}

	Super::Logout(Exiting);

	if (bHostLeft)
	{
		AssignHostIfNeeded(nullptr, false, ExitingPlayerState);
	}

	EvaluateLobbyState(ExitingPlayerState);
}

void AOnlineRacingLobbyGameMode::SetPlayerReady(AOnlineRacingPlayerState* PlayerState, const bool bIsReady)
{
	AOnlineRacingLobbyGameState* LobbyGameState = GetGameState<AOnlineRacingLobbyGameState>();
	if (!IsValid(LobbyGameState) || !IsValid(PlayerState) || !LobbyGameState->PlayerArray.Contains(PlayerState) || bTravelInProgress)
	{
		return;
	}

	PlayerState->SetLobbyReady(bIsReady);
	EvaluateLobbyState();
	FString ReadyState = TEXT("false");
	if (bIsReady)
	{
		ReadyState = TEXT("true");
	}

	UE_LOG(LogOnlineRacing, Log,
		TEXT("[Server][LobbyGameMode] Ready state changed. Player=%s Ready=%s"),
		*GetNameSafe(PlayerState),
		*ReadyState);
}

void AOnlineRacingLobbyGameMode::HandleStartMatchRequest(APlayerController* RequestingPlayer)
{
	if (!IsValid(RequestingPlayer) || bTravelInProgress)
	{
		return;
	}

	AOnlineRacingPlayerState* RequestingPlayerState = RequestingPlayer->GetPlayerState<AOnlineRacingPlayerState>();
	AOnlineRacingLobbyGameState* LobbyGameState = GetGameState<AOnlineRacingLobbyGameState>();
	if (!IsValid(RequestingPlayerState) || !RequestingPlayerState->IsLobbyHost())
	{
		UE_LOG(LogOnlineRacing, Warning, TEXT("[Server][LobbyGameMode] Start request rejected: requester is not the host. Player=%s"), *GetNameSafe(RequestingPlayerState));
		return;
	}

	if (!IsValid(LobbyGameState) || !LobbyGameState->CanStartMatch())
	{
		int32 ReadyPlayerCount = 0;
		int32 TotalPlayerCount = 0;
		if (IsValid(LobbyGameState))
		{
			ReadyPlayerCount = LobbyGameState->GetReadyPlayerCount();
			TotalPlayerCount = LobbyGameState->GetTotalPlayerCount();
		}

		UE_LOG(LogOnlineRacing, Log,
			TEXT("[Server][LobbyGameMode] Start request rejected: lobby is not ready. Ready=%d Total=%d Minimum=%d"),
			ReadyPlayerCount,
			TotalPlayerCount,
			MinimumPlayersToStart);
		return;
	}

	if (MatchMap.IsNull())
	{
		UE_LOG(LogOnlineRacing, Error, TEXT("[Server][LobbyGameMode] Start request rejected: MatchMap is not configured."));
		return;
	}

	const FString MatchMapPackageName = MatchMap.ToSoftObjectPath().GetLongPackageName();
	if (MatchMapPackageName.IsEmpty())
	{
		UE_LOG(LogOnlineRacing, Error, TEXT("[Server][LobbyGameMode] Start request rejected: MatchMap path is invalid. Path=%s"), *MatchMap.ToString());
		return;
	}

	bTravelInProgress = true;
	EvaluateLobbyState();
	MatchTravelDelay = FMath::Max(0.5f, MatchTravelDelay);

	UE_LOG(LogOnlineRacing, Log,
		TEXT("[Server][LobbyGameMode] Match start accepted. TravelDelay=%.2f Map=%s"),
		MatchTravelDelay,
		*MatchMapPackageName);

	GetWorldTimerManager().SetTimer(MatchTravelTimerHandle, this, &ThisClass::TravelToMatch, MatchTravelDelay, false);
}

void AOnlineRacingLobbyGameMode::AssignHostIfNeeded(AOnlineRacingPlayerState* JoiningPlayerState, const bool bPreferAsHost, const AOnlineRacingPlayerState* IgnoredPlayerState)
{
	AOnlineRacingLobbyGameState* LobbyGameState = GetGameState<AOnlineRacingLobbyGameState>();
	if (!IsValid(LobbyGameState))
	{
		return;
	}

	if (bPreferAsHost && IsValid(JoiningPlayerState))
	{
		for (APlayerState* PlayerState : LobbyGameState->PlayerArray)
		{
			AOnlineRacingPlayerState* OnlineRacingPlayerState = Cast<AOnlineRacingPlayerState>(PlayerState);
			if (IsValid(OnlineRacingPlayerState) && OnlineRacingPlayerState != JoiningPlayerState)
			{
				OnlineRacingPlayerState->SetLobbyHost(false);
			}
		}

		JoiningPlayerState->SetLobbyHost(true);
		return;
	}

	for (APlayerState* PlayerState : LobbyGameState->PlayerArray)
	{
		AOnlineRacingPlayerState* OnlineRacingPlayerState = Cast<AOnlineRacingPlayerState>(PlayerState);
		if (IsValid(OnlineRacingPlayerState)
			&& OnlineRacingPlayerState != JoiningPlayerState
			&& OnlineRacingPlayerState != IgnoredPlayerState
			&& OnlineRacingPlayerState->IsLobbyHost())
		{
			if (IsValid(JoiningPlayerState))
			{
				JoiningPlayerState->SetLobbyHost(false);
			}
			return;
		}
	}

	AOnlineRacingPlayerState* NewHostPlayerState = JoiningPlayerState;
	if (!IsValid(NewHostPlayerState))
	{
		for (APlayerState* PlayerState : LobbyGameState->PlayerArray)
		{
			AOnlineRacingPlayerState* CandidatePlayerState = Cast<AOnlineRacingPlayerState>(PlayerState);
			if (!IsValid(CandidatePlayerState) || CandidatePlayerState == IgnoredPlayerState)
			{
				continue;
			}

			NewHostPlayerState = CandidatePlayerState;
			break;
		}
	}

	if (IsValid(NewHostPlayerState))
	{
		NewHostPlayerState->SetLobbyHost(true);
	}
}

void AOnlineRacingLobbyGameMode::EvaluateLobbyState(const AOnlineRacingPlayerState* IgnoredPlayerState)
{
	AOnlineRacingLobbyGameState* LobbyGameState = GetGameState<AOnlineRacingLobbyGameState>();
	if (!IsValid(LobbyGameState))
	{
		return;
	}

	int32 TotalPlayerCount = 0;
	int32 ReadyPlayerCount = 0;
	for (APlayerState* PlayerState : LobbyGameState->PlayerArray)
	{
		const AOnlineRacingPlayerState* OnlineRacingPlayerState = Cast<AOnlineRacingPlayerState>(PlayerState);
		if (!IsValid(OnlineRacingPlayerState) || OnlineRacingPlayerState == IgnoredPlayerState)
		{
			continue;
		}

		++TotalPlayerCount;
		if (OnlineRacingPlayerState->IsLobbyReady())
		{
			++ReadyPlayerCount;
		}
	}

	bool bCanStart = false;
	if (!bTravelInProgress && TotalPlayerCount >= FMath::Max(1, MinimumPlayersToStart) && ReadyPlayerCount == TotalPlayerCount)
	{
		bCanStart = true;
	}

	LobbyGameState->SetLobbyState(ReadyPlayerCount, TotalPlayerCount, bCanStart, bTravelInProgress);
}

void AOnlineRacingLobbyGameMode::TravelToMatch()
{
	if (!bTravelInProgress || MatchMap.IsNull())
	{
		return;
	}

	const FString MatchMapPackageName = MatchMap.ToSoftObjectPath().GetLongPackageName();
	if (MatchMapPackageName.IsEmpty())
	{
		UE_LOG(LogOnlineRacing, Error, TEXT("[Server][LobbyGameMode] Match travel failed: map path became invalid. Path=%s"), *MatchMap.ToString());
		bTravelInProgress = false;
		EvaluateLobbyState();
		return;
	}

	UE_LOG(LogOnlineRacing, Log, TEXT("[Server][LobbyGameMode] Starting match travel. Map=%s"), *MatchMapPackageName);
	ProcessServerTravel(MatchMapPackageName);
}
