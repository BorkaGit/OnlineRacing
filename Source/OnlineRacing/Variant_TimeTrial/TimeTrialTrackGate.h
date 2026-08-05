// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TimeTrialTrackGate.generated.h"

class UBoxComponent;

UCLASS(Abstract)
class ATimeTrialTrackGate : public AActor
{
	GENERATED_BODY()

private:
	UPROPERTY(VisibleAnywhere, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBoxComponent> CollisionBox;

protected:
	UPROPERTY(EditInstanceOnly, Category = "Track Gate")
	bool bIsFinishLine = false;

	UPROPERTY(EditInstanceOnly, Category = "Track Gate")
	TObjectPtr<ATimeTrialTrackGate> NextMarker;

public:
	ATimeTrialTrackGate();
	ATimeTrialTrackGate* GetNextMarker() const;

protected:
	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;
};
