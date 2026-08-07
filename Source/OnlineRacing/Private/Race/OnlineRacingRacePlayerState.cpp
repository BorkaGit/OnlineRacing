#include "Race/OnlineRacingRacePlayerState.h"

#include "Net/UnrealNetwork.h"

void AOnlineRacingRacePlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AOnlineRacingRacePlayerState, CurrentLap);
	DOREPLIFETIME(AOnlineRacingRacePlayerState, LastCheckpointIndex);
	DOREPLIFETIME(AOnlineRacingRacePlayerState, NextCheckpointIndex);
	DOREPLIFETIME(AOnlineRacingRacePlayerState, bFinished);
	DOREPLIFETIME(AOnlineRacingRacePlayerState, FinishPosition);
	DOREPLIFETIME(AOnlineRacingRacePlayerState, FinishTimeSeconds);
}

void AOnlineRacingRacePlayerState::OnRep_Finished()
{
	RaceFinishedChanged.Broadcast(bFinished);
}

void AOnlineRacingRacePlayerState::FinishRace(const int32 NewFinishPosition, const double NewFinishTimeSeconds)
{
	bFinished = true;
	FinishPosition = NewFinishPosition;
	FinishTimeSeconds = NewFinishTimeSeconds;
	NextCheckpointIndex = INDEX_NONE;
	RaceFinishedChanged.Broadcast(bFinished);
	ForceNetUpdate();
}

void AOnlineRacingRacePlayerState::InitializeRaceProgress(const int32 InitialCheckpointIndex)
{
	CurrentLap = 1;
	LastCheckpointIndex = 0;
	NextCheckpointIndex = InitialCheckpointIndex;
	bFinished = false;
	FinishPosition = 0;
	FinishTimeSeconds = 0.0;
	RaceFinishedChanged.Broadcast(bFinished);
}

void AOnlineRacingRacePlayerState::SetRaceProgress(
	const int32 NewCurrentLap,
	const int32 NewLastCheckpointIndex,
	const int32 NewNextCheckpointIndex)
{
	CurrentLap = NewCurrentLap;
	LastCheckpointIndex = NewLastCheckpointIndex;
	NextCheckpointIndex = NewNextCheckpointIndex;
}
