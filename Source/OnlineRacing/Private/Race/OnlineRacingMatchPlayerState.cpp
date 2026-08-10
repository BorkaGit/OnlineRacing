#include "Race/OnlineRacingMatchPlayerState.h"

#include "Net/UnrealNetwork.h"

void AOnlineRacingMatchPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AOnlineRacingMatchPlayerState, CurrentLap);
	DOREPLIFETIME(AOnlineRacingMatchPlayerState, LastCheckpointIndex);
	DOREPLIFETIME(AOnlineRacingMatchPlayerState, NextCheckpointIndex);
	DOREPLIFETIME(AOnlineRacingMatchPlayerState, bFinished);
	DOREPLIFETIME(AOnlineRacingMatchPlayerState, FinishPosition);
	DOREPLIFETIME(AOnlineRacingMatchPlayerState, FinishTimeSeconds);
}

void AOnlineRacingMatchPlayerState::OnRep_Finished()
{
	RaceFinishedChanged.Broadcast(bFinished);
}

void AOnlineRacingMatchPlayerState::FinishRace(const int32 NewFinishPosition, const double NewFinishTimeSeconds)
{
	bFinished = true;
	FinishPosition = NewFinishPosition;
	FinishTimeSeconds = NewFinishTimeSeconds;
	NextCheckpointIndex = INDEX_NONE;
	RaceFinishedChanged.Broadcast(bFinished);
	ForceNetUpdate();
}

void AOnlineRacingMatchPlayerState::InitializeRaceProgress(const int32 InitialCheckpointIndex)
{
	CurrentLap = 1;
	LastCheckpointIndex = 0;
	NextCheckpointIndex = InitialCheckpointIndex;
	bFinished = false;
	FinishPosition = 0;
	FinishTimeSeconds = 0.0;
	RaceFinishedChanged.Broadcast(bFinished);
}

void AOnlineRacingMatchPlayerState::SetRaceProgress(
	const int32 NewCurrentLap,
	const int32 NewLastCheckpointIndex,
	const int32 NewNextCheckpointIndex)
{
	CurrentLap = NewCurrentLap;
	LastCheckpointIndex = NewLastCheckpointIndex;
	NextCheckpointIndex = NewNextCheckpointIndex;
}
