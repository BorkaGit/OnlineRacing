#include "Race/OnlineRacingMatchGameState.h"

#include "Net/UnrealNetwork.h"

void AOnlineRacingMatchGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AOnlineRacingMatchGameState, RacePhase);
	DOREPLIFETIME(AOnlineRacingMatchGameState, CountdownEndServerTime);
	DOREPLIFETIME(AOnlineRacingMatchGameState, RaceStartServerTime);
	DOREPLIFETIME(AOnlineRacingMatchGameState, TotalLaps);
	DOREPLIFETIME(AOnlineRacingMatchGameState, CheckpointCount);
	DOREPLIFETIME(AOnlineRacingMatchGameState, RaceResults);
}

double AOnlineRacingMatchGameState::GetCountdownTimeRemaining() const
{
	return FMath::Max(CountdownEndServerTime - GetServerWorldTimeSeconds(), 0.0);
}

void AOnlineRacingMatchGameState::OnRep_RacePhase()
{
	RacePhaseChanged.Broadcast(RacePhase);
}

void AOnlineRacingMatchGameState::OnRep_RaceResults()
{
	RaceResultsChanged.Broadcast(RaceResults);
}

void AOnlineRacingMatchGameState::AddRaceResult(const FOnlineRacingMatchResult& RaceResult)
{
	RaceResults.Add(RaceResult);
	RaceResultsChanged.Broadcast(RaceResults);
	ForceNetUpdate();
}

void AOnlineRacingMatchGameState::BeginCountdown(const double NewCountdownEndServerTime)
{
	CountdownEndServerTime = NewCountdownEndServerTime;
	SetRacePhase(EOnlineRacingMatchPhase::Countdown);
}

void AOnlineRacingMatchGameState::InitializeRace(const int32 NewTotalLaps, const int32 NewCheckpointCount)
{
	TotalLaps = NewTotalLaps;
	CheckpointCount = NewCheckpointCount;
	CountdownEndServerTime = 0.0;
	RaceStartServerTime = 0.0;
	RaceResults.Reset();
	RaceResultsChanged.Broadcast(RaceResults);
	SetRacePhase(EOnlineRacingMatchPhase::Waiting);
}

void AOnlineRacingMatchGameState::StartRace(const double NewRaceStartServerTime)
{
	RaceStartServerTime = NewRaceStartServerTime;
	SetRacePhase(EOnlineRacingMatchPhase::Racing);
}

void AOnlineRacingMatchGameState::SetRacePhase(const EOnlineRacingMatchPhase NewRacePhase)
{
	if (RacePhase == NewRacePhase)
	{
		return;
	}

	RacePhase = NewRacePhase;
	RacePhaseChanged.Broadcast(RacePhase);
}
