#pragma once

#include "GameFramework/GameStateBase.h"
#include "OnlineRacingMatchGameState.generated.h"

UENUM(BlueprintType)
enum class EOnlineRacingMatchPhase : uint8
{
	Waiting,
	Countdown,
	Racing,
	Finished
};

USTRUCT(BlueprintType)
struct ONLINERACING_API FOnlineRacingMatchResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Race|Results")
	int32 Position = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Race|Results")
	FString PlayerName;

	UPROPERTY(BlueprintReadOnly, Category = "Race|Results", meta = (Units = "s"))
	double FinishTimeSeconds = 0.0;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FOnlineRacingMatchPhaseChanged, EOnlineRacingMatchPhase);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnlineRacingMatchResultsChanged, const TArray<FOnlineRacingMatchResult>&);

UCLASS()
class ONLINERACING_API AOnlineRacingMatchGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	int32 GetCheckpointCount() const { return CheckpointCount; }
	double GetCountdownTimeRemaining() const;
	const TArray<FOnlineRacingMatchResult>& GetRaceResults() const { return RaceResults; }
	double GetRaceStartServerTime() const { return RaceStartServerTime; }
	EOnlineRacingMatchPhase GetRacePhase() const { return RacePhase; }
	int32 GetTotalLaps() const { return TotalLaps; }
	FOnlineRacingMatchPhaseChanged& OnRacePhaseChanged() { return RacePhaseChanged; }
	FOnlineRacingMatchResultsChanged& OnRaceResultsChanged() { return RaceResultsChanged; }

protected:
	UPROPERTY(ReplicatedUsing = OnRep_RacePhase, VisibleInstanceOnly, BlueprintReadOnly, Category = "Race")
	EOnlineRacingMatchPhase RacePhase = EOnlineRacingMatchPhase::Waiting;

	UPROPERTY(Replicated, VisibleInstanceOnly, BlueprintReadOnly, Category = "Race", meta = (Units = "s"))
	double CountdownEndServerTime = 0.0;

	UPROPERTY(Replicated, VisibleInstanceOnly, BlueprintReadOnly, Category = "Race", meta = (Units = "s"))
	double RaceStartServerTime = 0.0;

	UPROPERTY(Replicated, VisibleInstanceOnly, BlueprintReadOnly, Category = "Race", meta = (ClampMin = "1"))
	int32 TotalLaps = 1;

	UPROPERTY(Replicated, VisibleInstanceOnly, BlueprintReadOnly, Category = "Race", meta = (ClampMin = "2"))
	int32 CheckpointCount = 0;

	UPROPERTY(ReplicatedUsing = OnRep_RaceResults, VisibleInstanceOnly, BlueprintReadOnly, Category = "Race|Results")
	TArray<FOnlineRacingMatchResult> RaceResults;

private:
	friend class AOnlineRacingMatchGameMode;

	FOnlineRacingMatchPhaseChanged RacePhaseChanged;
	FOnlineRacingMatchResultsChanged RaceResultsChanged;

	UFUNCTION()
	void OnRep_RacePhase();

	UFUNCTION()
	void OnRep_RaceResults();

	void AddRaceResult(const FOnlineRacingMatchResult& RaceResult);
	void BeginCountdown(double NewCountdownEndServerTime);
	void InitializeRace(int32 NewTotalLaps, int32 NewCheckpointCount);
	void StartRace(double NewRaceStartServerTime);
	void SetRacePhase(EOnlineRacingMatchPhase NewRacePhase);
};
