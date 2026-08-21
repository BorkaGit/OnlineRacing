// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/PlayerControllers/OnlineRacingMainMenuPlayerController.h"

#include "OnlineRacing.h"
#include "UI/OnlineRacingSessionMenuWidget.h"


void AOnlineRacingMainMenuPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalPlayerController())
	{
		return;
	}

	const FInputModeUIOnly InputMode;
	SetInputMode(InputMode);
	SetShowMouseCursor(true);

	if (!IsValid(SessionMenuWidgetClass))
	{
		UE_LOG(LogOnlineRacing, Error, TEXT("[Local][MenuPlayerController] Session menu widget class is not configured. Controller=%s"), *GetNameSafe(this));
		return;
	}

	SessionMenuWidget = CreateWidget<UOnlineRacingSessionMenuWidget>(this, SessionMenuWidgetClass);
	if (!IsValid(SessionMenuWidget))
	{
		UE_LOG(LogOnlineRacing, Error, TEXT("[Local][MenuPlayerController] Failed to create the session menu. Controller=%s"), *GetNameSafe(this));
		return;
	}

	SessionMenuWidget->AddToPlayerScreen(0);
}
