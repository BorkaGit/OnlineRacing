#pragma once

#include "GameFramework/PlayerState.h"
#include "OnlineRacingRacePlayerState.generated.h"

UCLASS()
class ONLINERACING_API AOnlineRacingRacePlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	int32 GetCurrentLap() const { return CurrentLap; }
	int32 GetLastCheckpointIndex() const { return LastCheckpointIndex; }
	int32 GetNextCheckpointIndex() const { return NextCheckpointIndex; }
	bool HasFinishedRace() const { return bFinished; }

protected:
	UPROPERTY(Replicated, VisibleInstanceOnly, BlueprintReadOnly, Category = "Race|Progress")
	int32 CurrentLap = 0;

	UPROPERTY(Replicated, VisibleInstanceOnly, BlueprintReadOnly, Category = "Race|Progress")
	int32 LastCheckpointIndex = INDEX_NONE;

	UPROPERTY(Replicated, VisibleInstanceOnly, BlueprintReadOnly, Category = "Race|Progress")
	int32 NextCheckpointIndex = INDEX_NONE;

	UPROPERTY(Replicated, VisibleInstanceOnly, BlueprintReadOnly, Category = "Race|Progress")
	bool bFinished = false;

private:
	friend class AOnlineRacingRaceGameMode;

	void InitializeRaceProgress(int32 InitialCheckpointIndex);
	void SetRaceProgress(int32 NewCurrentLap, int32 NewLastCheckpointIndex, int32 NewNextCheckpointIndex, bool bNewFinished);
};
