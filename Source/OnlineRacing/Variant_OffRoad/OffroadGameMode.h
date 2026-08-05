// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "OffroadGameMode.generated.h"

UCLASS(Abstract)
class AOffroadGameMode : public AGameModeBase
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

	UPROPERTY(EditDefaultsOnly, Category = "Local Multiplayer", meta = (ClampMin = "1", ClampMax = "4"))
	int32 NumberOfLocalPlayers = 1;

	int32 CurrentPlayerStartAssignment = 0;
};
