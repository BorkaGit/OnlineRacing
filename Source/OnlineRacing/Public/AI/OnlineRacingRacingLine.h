// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Actor.h"
#include "OnlineRacingRacingLine.generated.h"

class USplineComponent;

UCLASS()
class ONLINERACING_API AOnlineRacingRacingLine : public AActor
{
	GENERATED_BODY()
	
public:	
	AOnlineRacingRacingLine();
	
	FORCEINLINE USplineComponent* GetSplineComponent() const { return SplineComponent; }
	
private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USplineComponent> SplineComponent;
};