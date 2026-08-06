#include "Race/OnlineRacingRaceGameState.h"

#include "Net/UnrealNetwork.h"

void AOnlineRacingRaceGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AOnlineRacingRaceGameState, RacePhase);
	DOREPLIFETIME(AOnlineRacingRaceGameState, CountdownEndServerTime);
	DOREPLIFETIME(AOnlineRacingRaceGameState, TotalLaps);
	DOREPLIFETIME(AOnlineRacingRaceGameState, CheckpointCount);
}

double AOnlineRacingRaceGameState::GetCountdownTimeRemaining() const
{
	return FMath::Max(CountdownEndServerTime - GetServerWorldTimeSeconds(), 0.0);
}

void AOnlineRacingRaceGameState::OnRep_RacePhase()
{
	RacePhaseChanged.Broadcast(RacePhase);
}

void AOnlineRacingRaceGameState::BeginCountdown(const double NewCountdownEndServerTime)
{
	CountdownEndServerTime = NewCountdownEndServerTime;
	SetRacePhase(EOnlineRacingRacePhase::Countdown);
}

void AOnlineRacingRaceGameState::InitializeRace(const int32 NewTotalLaps, const int32 NewCheckpointCount)
{
	TotalLaps = NewTotalLaps;
	CheckpointCount = NewCheckpointCount;
	CountdownEndServerTime = 0.0;
	SetRacePhase(EOnlineRacingRacePhase::Waiting);
}

void AOnlineRacingRaceGameState::SetRacePhase(const EOnlineRacingRacePhase NewRacePhase)
{
	if (RacePhase == NewRacePhase)
	{
		return;
	}

	RacePhase = NewRacePhase;
	RacePhaseChanged.Broadcast(RacePhase);
}
