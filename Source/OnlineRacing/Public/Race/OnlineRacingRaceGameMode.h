#pragma once

#include "OnlineRacingGameMode.h"
#include "OnlineRacingRaceGameMode.generated.h"

class AOnlineRacingPawn;
class AOnlineRacingRaceCheckpoint;
class AOnlineRacingRacePlayerState;
class AController;
class APlayerController;

UCLASS(Abstract)
class ONLINERACING_API AOnlineRacingRaceGameMode : public AOnlineRacingGameMode
{
	GENERATED_BODY()

public:
	AOnlineRacingRaceGameMode();

	void HandleCheckpointReached(const AOnlineRacingRaceCheckpoint& Checkpoint, const AOnlineRacingPawn& VehiclePawn);
	bool HandleRespawnRequest(AController& Controller);

protected:
	virtual void BeginPlay() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Race", meta = (ClampMin = "1"))
	int32 TotalLaps = 3;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Race|Countdown", meta = (ClampMin = "1.0", Units = "s"))
	float CountdownDuration = 3.f;

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<AOnlineRacingRaceCheckpoint>> RaceCheckpoints;

	bool bCheckpointConfigurationValid = false;
	FTimerHandle CountdownTimer;
	int32 NextFinishPosition = 1;

	bool AreAllPlayersFinished() const;
	void BeginCountdown();
	bool DiscoverRaceCheckpoints();
	void InitializePlayerState(AOnlineRacingRacePlayerState& RacePlayerState) const;
	void RecordPlayerFinish(AOnlineRacingRacePlayerState& RacePlayerState);
	void StartRace();
};
