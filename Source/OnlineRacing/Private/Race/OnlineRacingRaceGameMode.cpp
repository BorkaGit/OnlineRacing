#include "Race/OnlineRacingRaceGameMode.h"

#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

#include "OnlineRacing.h"
#include "OnlineRacingPawn.h"
#include "Race/OnlineRacingRaceCheckpoint.h"
#include "Race/OnlineRacingRaceGameState.h"
#include "Race/OnlineRacingRacePlayerState.h"

AOnlineRacingRaceGameMode::AOnlineRacingRaceGameMode()
{
	GameStateClass = AOnlineRacingRaceGameState::StaticClass();
	PlayerStateClass = AOnlineRacingRacePlayerState::StaticClass();
}

void AOnlineRacingRaceGameMode::BeginPlay()
{
	Super::BeginPlay();

	bCheckpointConfigurationValid = DiscoverRaceCheckpoints();
	if (!bCheckpointConfigurationValid)
	{
		return;
	}

	AOnlineRacingRaceGameState* const RaceGameState = GetGameState<AOnlineRacingRaceGameState>();
	if (!IsValid(RaceGameState))
	{
		UE_LOG(LogOnlineRacing, Error, TEXT("[Server][Race] RaceGameState is unavailable."));
		return;
	}

	RaceGameState->InitializeRace(TotalLaps, RaceCheckpoints.Num());
	NextFinishPosition = 1;
	for (APlayerState* const PlayerState : RaceGameState->PlayerArray)
	{
		AOnlineRacingRacePlayerState* const RacePlayerState = Cast<AOnlineRacingRacePlayerState>(PlayerState);
		if (IsValid(RacePlayerState))
		{
			InitializePlayerState(*RacePlayerState);
		}
	}

	BeginCountdown();
}

void AOnlineRacingRaceGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (!bCheckpointConfigurationValid || !IsValid(NewPlayer))
	{
		return;
	}

	AOnlineRacingRacePlayerState* const RacePlayerState = NewPlayer->GetPlayerState<AOnlineRacingRacePlayerState>();
	if (IsValid(RacePlayerState))
	{
		InitializePlayerState(*RacePlayerState);
	}
}

void AOnlineRacingRaceGameMode::HandleCheckpointReached(
	const AOnlineRacingRaceCheckpoint& Checkpoint,
	const AOnlineRacingPawn& VehiclePawn)
{
	if (!bCheckpointConfigurationValid)
	{
		return;
	}

	AOnlineRacingRaceGameState* const RaceGameState = GetGameState<AOnlineRacingRaceGameState>();
	if (!IsValid(RaceGameState) || RaceGameState->GetRacePhase() != EOnlineRacingRacePhase::Racing)
	{
		return;
	}

	AOnlineRacingRacePlayerState* const RacePlayerState = VehiclePawn.GetPlayerState<AOnlineRacingRacePlayerState>();
	if (!IsValid(RacePlayerState) || RacePlayerState->HasFinishedRace())
	{
		return;
	}

	const int32 CheckpointIndex = Checkpoint.GetCheckpointIndex();
	if (CheckpointIndex != RacePlayerState->GetNextCheckpointIndex())
	{
		return;
	}

	int32 CurrentLap = RacePlayerState->GetCurrentLap();
	int32 NextCheckpointIndex = CheckpointIndex + 1;
	bool bFinished = false;

	if (NextCheckpointIndex >= RaceCheckpoints.Num())
	{
		NextCheckpointIndex = 0;
	}

	if (CheckpointIndex == 0)
	{
		if (CurrentLap >= TotalLaps)
		{
			bFinished = true;
			NextCheckpointIndex = INDEX_NONE;
			UE_LOG(LogOnlineRacing, Display, TEXT("[Server][Race] %s finished the race on lap %d."), *GetNameSafe(RacePlayerState), CurrentLap);
		}
		else
		{
			++CurrentLap;
			NextCheckpointIndex = 1;
			UE_LOG(LogOnlineRacing, Display, TEXT("[Server][Race] %s started lap %d."), *GetNameSafe(RacePlayerState), CurrentLap);
		}
	}

	RacePlayerState->SetRaceProgress(CurrentLap, CheckpointIndex, NextCheckpointIndex);
	if (bFinished)
	{
		RecordPlayerFinish(*RacePlayerState);
	}

	if (!bFinished || !AreAllPlayersFinished())
	{
		return;
	}

	if (IsValid(RaceGameState))
	{
		RaceGameState->SetRacePhase(EOnlineRacingRacePhase::Finished);
	}
}

bool AOnlineRacingRaceGameMode::HandleRespawnRequest(AController& Controller)
{
	if (!HasAuthority() || !bCheckpointConfigurationValid)
	{
		return false;
	}

	const AOnlineRacingRacePlayerState* const RacePlayerState = Controller.GetPlayerState<AOnlineRacingRacePlayerState>();
	AOnlineRacingPawn* const VehiclePawn = Cast<AOnlineRacingPawn>(Controller.GetPawn());
	if (!IsValid(RacePlayerState) || !IsValid(VehiclePawn))
	{
		UE_LOG(LogOnlineRacing, Warning, TEXT("[Server][Race] Rejected respawn for %s because its race state or vehicle is unavailable."), *GetNameSafe(&Controller));
		return false;
	}

	const int32 CheckpointIndex = RacePlayerState->GetLastCheckpointIndex();
	if (!RaceCheckpoints.IsValidIndex(CheckpointIndex) || !IsValid(RaceCheckpoints[CheckpointIndex]))
	{
		UE_LOG(LogOnlineRacing, Warning, TEXT("[Server][Race] Rejected respawn for %s because checkpoint index %d is invalid."), *GetNameSafe(&Controller), CheckpointIndex);
		return false;
	}

	const AOnlineRacingRaceCheckpoint* const Checkpoint = RaceCheckpoints[CheckpointIndex];
	VehiclePawn->RespawnVehicleAtTransform(Checkpoint->GetRespawnTransform());

	UE_LOG(LogOnlineRacing, Display, TEXT("[Server][Race] Respawned %s at checkpoint %d."), *GetNameSafe(VehiclePawn), CheckpointIndex);
	
	return true;
}

void AOnlineRacingRaceGameMode::BeginCountdown()
{
	if (!HasAuthority())
	{
		return;
	}

	AOnlineRacingRaceGameState* const RaceGameState = GetGameState<AOnlineRacingRaceGameState>();
	if (!IsValid(RaceGameState))
	{
		UE_LOG(LogOnlineRacing, Error, TEXT("[Server][Race] Cannot begin countdown because RaceGameState is unavailable."));
		return;
	}

	const double CountdownEndServerTime = RaceGameState->GetServerWorldTimeSeconds() + CountdownDuration;
	RaceGameState->BeginCountdown(CountdownEndServerTime);
	GetWorldTimerManager().SetTimer(CountdownTimer, this, &AOnlineRacingRaceGameMode::StartRace, CountdownDuration, false);

	UE_LOG(LogOnlineRacing, Display, TEXT("[Server][Race] Countdown started and will end at server time %.3f."), CountdownEndServerTime);
}

void AOnlineRacingRaceGameMode::StartRace()
{
	if (!HasAuthority())
	{
		return;
	}

	AOnlineRacingRaceGameState* const RaceGameState = GetGameState<AOnlineRacingRaceGameState>();
	if (!IsValid(RaceGameState) || RaceGameState->GetRacePhase() != EOnlineRacingRacePhase::Countdown)
	{
		UE_LOG(LogOnlineRacing, Warning, TEXT("[Server][Race] Rejected countdown completion because the race is not in Countdown phase."));
		return;
	}

	const double RaceStartServerTime = RaceGameState->GetServerWorldTimeSeconds();
	RaceGameState->StartRace(RaceStartServerTime);
	UE_LOG(LogOnlineRacing, Display, TEXT("[Server][Race] Countdown completed; race started."));
}

bool AOnlineRacingRaceGameMode::DiscoverRaceCheckpoints()
{
	TArray<AActor*> FoundCheckpointActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AOnlineRacingRaceCheckpoint::StaticClass(), FoundCheckpointActors);

	RaceCheckpoints.Reset(FoundCheckpointActors.Num());
	for (AActor* const FoundActor : FoundCheckpointActors)
	{
		AOnlineRacingRaceCheckpoint* const Checkpoint = Cast<AOnlineRacingRaceCheckpoint>(FoundActor);
		if (IsValid(Checkpoint))
		{
			RaceCheckpoints.Add(Checkpoint);
		}
	}

	if (RaceCheckpoints.Num() < 2)
	{
		UE_LOG(LogOnlineRacing, Error, TEXT("[Server][Race] At least two checkpoints are required; found %d."), RaceCheckpoints.Num());
		return false;
	}

	RaceCheckpoints.Sort([](
		const AOnlineRacingRaceCheckpoint& Left,
		const AOnlineRacingRaceCheckpoint& Right)
	{
		return Left.GetCheckpointIndex() < Right.GetCheckpointIndex();
	});

	for (int32 ExpectedIndex = 0; ExpectedIndex < RaceCheckpoints.Num(); ++ExpectedIndex)
	{
		const AOnlineRacingRaceCheckpoint* const Checkpoint = RaceCheckpoints[ExpectedIndex];
		if (!IsValid(Checkpoint) || Checkpoint->GetCheckpointIndex() != ExpectedIndex)
		{
			int32 FoundIndex = INDEX_NONE;
			if (IsValid(Checkpoint))
			{
				FoundIndex = Checkpoint->GetCheckpointIndex();
			}

			UE_LOG(
				LogOnlineRacing,
				Error,
				TEXT("[Server][Race] Checkpoint indices must be unique and contiguous from 0. Expected index %d, found %d on %s."),
				ExpectedIndex,
				FoundIndex,
				*GetNameSafe(Checkpoint));
			return false;
		}
	}

	return true;
}

void AOnlineRacingRaceGameMode::InitializePlayerState(AOnlineRacingRacePlayerState& RacePlayerState) const
{
	RacePlayerState.InitializeRaceProgress(1);
}

void AOnlineRacingRaceGameMode::RecordPlayerFinish(AOnlineRacingRacePlayerState& RacePlayerState)
{
	AOnlineRacingRaceGameState* const RaceGameState = GetGameState<AOnlineRacingRaceGameState>();
	if (!IsValid(RaceGameState))
	{
		UE_LOG(LogOnlineRacing, Error, TEXT("[Server][Race] Cannot record finish because RaceGameState is unavailable."));
		return;
	}

	const double FinishTimeSeconds = FMath::Max(
		RaceGameState->GetServerWorldTimeSeconds() - RaceGameState->GetRaceStartServerTime(),
		0.0);
	const int32 FinishPosition = NextFinishPosition;
	++NextFinishPosition;

	RacePlayerState.FinishRace(FinishPosition, FinishTimeSeconds);

	FOnlineRacingRaceResult RaceResult;
	RaceResult.Position = FinishPosition;
	RaceResult.PlayerName = RacePlayerState.GetPlayerName();
	if (RaceResult.PlayerName.IsEmpty())
	{
		RaceResult.PlayerName = FString::Printf(TEXT("Racer %d"), FinishPosition);
	}
	RaceResult.FinishTimeSeconds = FinishTimeSeconds;
	RaceGameState->AddRaceResult(RaceResult);

	UE_LOG(
		LogOnlineRacing,
		Display,
		TEXT("[Server][Race] %s finished in position %d with time %.3f s."),
		*GetNameSafe(&RacePlayerState),
		FinishPosition,
		FinishTimeSeconds);
}

bool AOnlineRacingRaceGameMode::AreAllPlayersFinished() const
{
	const AOnlineRacingRaceGameState* const RaceGameState = GetGameState<AOnlineRacingRaceGameState>();
	if (!IsValid(RaceGameState))
	{
		return false;
	}

	bool bFoundRacingPlayer = false;
	for (const APlayerState* const PlayerState : RaceGameState->PlayerArray)
	{
		const AOnlineRacingRacePlayerState* const RacePlayerState = Cast<AOnlineRacingRacePlayerState>(PlayerState);
		if (!IsValid(RacePlayerState))
		{
			continue;
		}

		bFoundRacingPlayer = true;
		if (!RacePlayerState->HasFinishedRace())
		{
			return false;
		}
	}

	return bFoundRacingPlayer;
}
