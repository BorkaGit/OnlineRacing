// Copyright Epic Games, Inc. All Rights Reserved.

#include "OnlineRacingGameMode.h"
#include "OnlineRacingPlayerController.h"

AOnlineRacingGameMode::AOnlineRacingGameMode()
{
	PlayerControllerClass = AOnlineRacingPlayerController::StaticClass();
}
