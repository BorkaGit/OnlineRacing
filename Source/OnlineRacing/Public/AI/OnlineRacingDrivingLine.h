// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
#include "OnlineRacingDrivingLine.generated.h"

class USplineComponent;

UCLASS()
class ONLINERACING_API AOnlineRacingDrivingLine : public AActor
{
	GENERATED_BODY()

public:
	AOnlineRacingDrivingLine();

	FORCEINLINE USplineComponent* GetSplineComponent() const { return SplineComponent; }

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USplineComponent> SplineComponent;
};
