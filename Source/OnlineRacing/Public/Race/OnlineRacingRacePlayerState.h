#pragma once

#include "GameFramework/PlayerState.h"
#include "OnlineRacingRacePlayerState.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnlineRacingPlayerFinishedChanged, bool);

UCLASS()
class ONLINERACING_API AOnlineRacingRacePlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	int32 GetCurrentLap() const { return CurrentLap; }
	int32 GetLastCheckpointIndex() const { return LastCheckpointIndex; }
	int32 GetNextCheckpointIndex() const { return NextCheckpointIndex; }
	int32 GetFinishPosition() const { return FinishPosition; }
	double GetFinishTimeSeconds() const { return FinishTimeSeconds; }
	bool HasFinishedRace() const { return bFinished; }
	FOnlineRacingPlayerFinishedChanged& OnRaceFinishedChanged() { return RaceFinishedChanged; }

protected:
	UPROPERTY(Replicated, VisibleInstanceOnly, BlueprintReadOnly, Category = "Race|Progress")
	int32 CurrentLap = 0;

	UPROPERTY(Replicated, VisibleInstanceOnly, BlueprintReadOnly, Category = "Race|Progress")
	int32 LastCheckpointIndex = INDEX_NONE;

	UPROPERTY(Replicated, VisibleInstanceOnly, BlueprintReadOnly, Category = "Race|Progress")
	int32 NextCheckpointIndex = INDEX_NONE;

	UPROPERTY(ReplicatedUsing = OnRep_Finished, VisibleInstanceOnly, BlueprintReadOnly, Category = "Race|Progress")
	bool bFinished = false;

	UPROPERTY(Replicated, VisibleInstanceOnly, BlueprintReadOnly, Category = "Race|Results")
	int32 FinishPosition = 0;

	UPROPERTY(Replicated, VisibleInstanceOnly, BlueprintReadOnly, Category = "Race|Results", meta = (Units = "s"))
	double FinishTimeSeconds = 0.0;

private:
	friend class AOnlineRacingRaceGameMode;

	FOnlineRacingPlayerFinishedChanged RaceFinishedChanged;

	UFUNCTION()
	void OnRep_Finished();

	void FinishRace(int32 NewFinishPosition, double NewFinishTimeSeconds);
	void InitializeRaceProgress(int32 InitialCheckpointIndex);
	void SetRaceProgress(int32 NewCurrentLap, int32 NewLastCheckpointIndex, int32 NewNextCheckpointIndex);
};
