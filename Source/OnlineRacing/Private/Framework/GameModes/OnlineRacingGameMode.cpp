// Copyright Epic Games, Inc. All Rights Reserved.

#include "Framework/GameModes/OnlineRacingGameMode.h"
#include "Framework/PlayerControllers/OnlineRacingMatchPlayerController.h"

AOnlineRacingGameMode::AOnlineRacingGameMode()
{
	PlayerControllerClass = AOnlineRacingMatchPlayerController::StaticClass();
}
