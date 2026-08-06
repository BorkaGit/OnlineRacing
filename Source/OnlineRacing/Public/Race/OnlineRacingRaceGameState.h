#pragma once

#include "GameFramework/GameStateBase.h"
#include "OnlineRacingRaceGameState.generated.h"

UENUM(BlueprintType)
enum class EOnlineRacingRacePhase : uint8
{
	Waiting,
	Countdown,
	Racing,
	Finished
};

DECLARE_MULTICAST_DELEGATE_OneParam(FOnlineRacingRacePhaseChanged, EOnlineRacingRacePhase);

UCLASS()
class ONLINERACING_API AOnlineRacingRaceGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	int32 GetCheckpointCount() const { return CheckpointCount; }
	double GetCountdownTimeRemaining() const;
	EOnlineRacingRacePhase GetRacePhase() const { return RacePhase; }
	int32 GetTotalLaps() const { return TotalLaps; }
	FOnlineRacingRacePhaseChanged& OnRacePhaseChanged() { return RacePhaseChanged; }

protected:
	UPROPERTY(ReplicatedUsing = OnRep_RacePhase, VisibleInstanceOnly, BlueprintReadOnly, Category = "Race")
	EOnlineRacingRacePhase RacePhase = EOnlineRacingRacePhase::Waiting;

	UPROPERTY(Replicated, VisibleInstanceOnly, BlueprintReadOnly, Category = "Race", meta = (Units = "s"))
	double CountdownEndServerTime = 0.0;

	UPROPERTY(Replicated, VisibleInstanceOnly, BlueprintReadOnly, Category = "Race", meta = (ClampMin = "1"))
	int32 TotalLaps = 1;

	UPROPERTY(Replicated, VisibleInstanceOnly, BlueprintReadOnly, Category = "Race", meta = (ClampMin = "2"))
	int32 CheckpointCount = 0;

private:
	friend class AOnlineRacingRaceGameMode;

	FOnlineRacingRacePhaseChanged RacePhaseChanged;

	UFUNCTION()
	void OnRep_RacePhase();

	void BeginCountdown(double NewCountdownEndServerTime);
	void InitializeRace(int32 NewTotalLaps, int32 NewCheckpointCount);
	void SetRacePhase(EOnlineRacingRacePhase NewRacePhase);
};
