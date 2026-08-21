// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "OnlineRacingMainMenuPlayerController.generated.h"

class UOnlineRacingSessionMenuWidget;

UCLASS()
class ONLINERACING_API AOnlineRacingMainMenuPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Session")
	TSubclassOf<UOnlineRacingSessionMenuWidget> SessionMenuWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UOnlineRacingSessionMenuWidget> SessionMenuWidget;


};
