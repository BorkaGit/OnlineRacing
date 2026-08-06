#pragma once

#include "GameFramework/Actor.h"
#include "OnlineRacingRaceCheckpoint.generated.h"

class UArrowComponent;
class UBoxComponent;
class UPrimitiveComponent;
struct FHitResult;

UCLASS(Abstract)
class ONLINERACING_API AOnlineRacingRaceCheckpoint : public AActor
{
	GENERATED_BODY()

public:
	AOnlineRacingRaceCheckpoint();

	int32 GetCheckpointIndex() const { return CheckpointIndex; }
	FTransform GetRespawnTransform() const;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBoxComponent> CollisionBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UArrowComponent> RespawnPoint;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Race|Checkpoint", meta = (AllowPrivateAccess = "true", ClampMin = "0"))
	int32 CheckpointIndex = 0;

	UFUNCTION()
	void HandleCollisionBoxBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);
};
