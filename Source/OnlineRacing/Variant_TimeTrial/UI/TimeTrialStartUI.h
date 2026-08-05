// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TimeTrialStartUI.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FCountdownFinishedDelegate);

UCLASS(Abstract)
class UTimeTrialStartUI : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FCountdownFinishedDelegate OnCountdownFinished;

	void StartCountdown();

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "Countdown", meta = (DisplayName = "Start Countdown"))
	void BP_StartCountdown();

	UFUNCTION(BlueprintCallable, Category = "Countdown")
	void FinishCountdown();
};
