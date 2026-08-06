#include "Race/OnlineRacingRacePlayerState.h"

#include "Net/UnrealNetwork.h"

void AOnlineRacingRacePlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AOnlineRacingRacePlayerState, CurrentLap);
	DOREPLIFETIME(AOnlineRacingRacePlayerState, LastCheckpointIndex);
	DOREPLIFETIME(AOnlineRacingRacePlayerState, NextCheckpointIndex);
	DOREPLIFETIME(AOnlineRacingRacePlayerState, bFinished);
}

void AOnlineRacingRacePlayerState::InitializeRaceProgress(const int32 InitialCheckpointIndex)
{
	CurrentLap = 1;
	LastCheckpointIndex = 0;
	NextCheckpointIndex = InitialCheckpointIndex;
	bFinished = false;
}

void AOnlineRacingRacePlayerState::SetRaceProgress(
	const int32 NewCurrentLap,
	const int32 NewLastCheckpointIndex,
	const int32 NewNextCheckpointIndex,
	const bool bNewFinished)
{
	CurrentLap = NewCurrentLap;
	LastCheckpointIndex = NewLastCheckpointIndex;
	NextCheckpointIndex = NewNextCheckpointIndex;
	bFinished = bNewFinished;
}
