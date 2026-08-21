#include "Framework/PlayerStates/OnlineRacingPlayerState.h"

#include "Net/UnrealNetwork.h"

void AOnlineRacingPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(AOnlineRacingPlayerState, bIsLobbyReady, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AOnlineRacingPlayerState, bIsLobbyHost, COND_OwnerOnly);

	DOREPLIFETIME(AOnlineRacingPlayerState, CurrentLap);
	DOREPLIFETIME(AOnlineRacingPlayerState, LastCheckpointIndex);
	DOREPLIFETIME(AOnlineRacingPlayerState, NextCheckpointIndex);
	DOREPLIFETIME(AOnlineRacingPlayerState, bFinished);
	DOREPLIFETIME(AOnlineRacingPlayerState, FinishPosition);
	DOREPLIFETIME(AOnlineRacingPlayerState, FinishTimeSeconds);
}

void AOnlineRacingPlayerState::SetLobbyHost(bool bNewIsHost)
{
	if (!HasAuthority() || bIsLobbyHost == bNewIsHost)
	{
		return;
	}

	bIsLobbyHost = bNewIsHost;
	OnRep_LobbyState();
	ForceNetUpdate();
}

void AOnlineRacingPlayerState::SetLobbyReady(bool bNewIsReady)
{
	if (!HasAuthority() || bIsLobbyReady == bNewIsReady)
	{
		return;
	}

	bIsLobbyReady = bNewIsReady;
	OnRep_LobbyState();
	ForceNetUpdate();
}

void AOnlineRacingPlayerState::OnRep_LobbyState()
{
	OnLobbyPlayerStateChanged.Broadcast(bIsLobbyReady, bIsLobbyHost);
}

void AOnlineRacingPlayerState::OnRep_Finished()
{
	RaceFinishedChanged.Broadcast(bFinished);
}

void AOnlineRacingPlayerState::FinishRace(const int32 NewFinishPosition, const double NewFinishTimeSeconds)
{
	bFinished = true;
	FinishPosition = NewFinishPosition;
	FinishTimeSeconds = NewFinishTimeSeconds;
	NextCheckpointIndex = INDEX_NONE;
	RaceFinishedChanged.Broadcast(bFinished);
	ForceNetUpdate();
}

void AOnlineRacingPlayerState::InitializeRaceProgress(const int32 InitialCheckpointIndex)
{
	CurrentLap = 1;
	LastCheckpointIndex = 0;
	NextCheckpointIndex = InitialCheckpointIndex;
	bFinished = false;
	FinishPosition = 0;
	FinishTimeSeconds = 0.0;
	RaceFinishedChanged.Broadcast(bFinished);
}

void AOnlineRacingPlayerState::SetRaceProgress(
	const int32 NewCurrentLap,
	const int32 NewLastCheckpointIndex,
	const int32 NewNextCheckpointIndex)
{
	CurrentLap = NewCurrentLap;
	LastCheckpointIndex = NewLastCheckpointIndex;
	NextCheckpointIndex = NewNextCheckpointIndex;
}
