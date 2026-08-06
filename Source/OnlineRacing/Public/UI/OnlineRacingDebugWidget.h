// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Blueprint/UserWidget.h"
#include "OnlineRacingDebugWidget.generated.h"

class AOnlineRacingPawn;
class UTextBlock;

UCLASS(Abstract)
class ONLINERACING_API UOnlineRacingDebugWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void UpdateDebugData(const AOnlineRacingPawn& VehiclePawn);

protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget), Category = "Debug")
	TObjectPtr<UTextBlock> DebugText;
};
