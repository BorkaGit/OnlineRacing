#include "Race/OnlineRacingRaceGameState.h"

#include "Net/UnrealNetwork.h"

void AOnlineRacingRaceGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AOnlineRacingRaceGameState, RacePhase);
	DOREPLIFETIME(AOnlineRacingRaceGameState, CountdownEndServerTime);
	DOREPLIFETIME(AOnlineRacingRaceGameState, RaceStartServerTime);
	DOREPLIFETIME(AOnlineRacingRaceGameState, TotalLaps);
	DOREPLIFETIME(AOnlineRacingRaceGameState, CheckpointCount);
	DOREPLIFETIME(AOnlineRacingRaceGameState, RaceResults);
}

double AOnlineRacingRaceGameState::GetCountdownTimeRemaining() const
{
	return FMath::Max(CountdownEndServerTime - GetServerWorldTimeSeconds(), 0.0);
}

void AOnlineRacingRaceGameState::OnRep_RacePhase()
{
	RacePhaseChanged.Broadcast(RacePhase);
}

void AOnlineRacingRaceGameState::OnRep_RaceResults()
{
	RaceResultsChanged.Broadcast(RaceResults);
}

void AOnlineRacingRaceGameState::AddRaceResult(const FOnlineRacingRaceResult& RaceResult)
{
	RaceResults.Add(RaceResult);
	RaceResultsChanged.Broadcast(RaceResults);
	ForceNetUpdate();
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
	RaceStartServerTime = 0.0;
	RaceResults.Reset();
	RaceResultsChanged.Broadcast(RaceResults);
	SetRacePhase(EOnlineRacingRacePhase::Waiting);
}

void AOnlineRacingRaceGameState::StartRace(const double NewRaceStartServerTime)
{
	RaceStartServerTime = NewRaceStartServerTime;
	SetRacePhase(EOnlineRacingRacePhase::Racing);
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
