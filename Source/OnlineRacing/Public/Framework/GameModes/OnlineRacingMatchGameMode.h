#pragma once

#include "OnlineRacingGameMode.h"
#include "OnlineRacingMatchGameMode.generated.h"

class AOnlineRacingAIController;
class AOnlineRacingPawn;
class AOnlineRacingGridStart;
class AOnlineRacingCheckpoint;
class AOnlineRacingPlayerState;
class AController;
class APlayerController;

UCLASS(Abstract)
class ONLINERACING_API AOnlineRacingMatchGameMode : public AOnlineRacingGameMode
{
	GENERATED_BODY()

public:
	AOnlineRacingMatchGameMode();

	void HandleCheckpointReached(const AOnlineRacingCheckpoint& Checkpoint, const AOnlineRacingPawn& VehiclePawn);
	bool HandleRespawnRequest(AController& Controller);

protected:

	virtual void BeginPlay() override;

	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

	virtual void PostLogin(APlayerController* NewPlayer) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Race", meta = (ClampMin = "1"))
	int32 TotalLaps = 3;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Race|Countdown", meta = (ClampMin = "1.0", Units = "s"))
	float CountdownDuration = 3.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Race|AI", meta = (ClampMin = "0", ClampMax = "3"))
	int32 BotCount = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Race|AI", meta = (ClampMin = "1", ClampMax = "3"))
	int32 AILaneCount = 2;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Race|AI")
	TSubclassOf<AOnlineRacingAIController> AIControllerClass;

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<AOnlineRacingCheckpoint>> RaceCheckpoints;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AOnlineRacingGridStart>> RacePlayerStarts;

	bool bCheckpointConfigurationValid = false;
	FTimerHandle CountdownTimer;
	int32 NextFinishPosition = 1;

	bool SpawnBots();
	bool AreAllPlayersFinished() const;
	void BeginCountdown();
	bool DiscoverRaceCheckpoints();
	bool DiscoverRacePlayerStarts();
	void InitializePlayerState(AOnlineRacingPlayerState& RacePlayerState) const;
	void RecordPlayerFinish(AOnlineRacingPlayerState& RacePlayerState);
	void StartRace();
};
