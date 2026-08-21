// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "OnlineRacingUI.generated.h"

UCLASS(Abstract)
class UOnlineRacingUI : public UUserWidget
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vehicle")
	bool bIsMPH = false;

public:
	void UpdateSpeed(float NewSpeed);
	void UpdateGear(int32 NewGear);

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "Vehicle")
	void OnSpeedUpdate(float NewSpeed);

	UFUNCTION(BlueprintImplementableEvent, Category = "Vehicle")
	void OnGearUpdate(int32 NewGear);
};
