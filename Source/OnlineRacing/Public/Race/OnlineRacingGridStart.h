// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/PlayerStart.h"
#include "OnlineRacingGridStart.generated.h"

UCLASS()
class ONLINERACING_API AOnlineRacingGridStart : public APlayerStart
{
	GENERATED_BODY()

public:
	FORCEINLINE int32 GetGridIndex() const { return GridIndex; }

protected:
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Race|Start", meta = (ClampMin = "0"))
	int32 GridIndex = 0;
};