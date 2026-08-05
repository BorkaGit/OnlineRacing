// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TimeTrialGameMode.generated.h"

class ATimeTrialTrackGate;

UCLASS(Abstract)
class ATimeTrialGameMode : public AGameModeBase
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Time Trial")
	FName FinishTag;

	UPROPERTY(EditDefaultsOnly, Category = "Time Trial", meta = (ClampMin = "1"))
	int32 Laps = 3;

	UPROPERTY(Transient)
	TObjectPtr<ATimeTrialTrackGate> FinishLineMarker;

	UPROPERTY(EditDefaultsOnly, Category = "Local Multiplayer", meta = (ClampMin = "1", ClampMax = "4"))
	int32 NumberOfLocalPlayers = 1;

	int32 CurrentPlayerStartAssignment = 0;

	virtual void BeginPlay() override;
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

public:
	ATimeTrialTrackGate* GetFinishLine() const;
	int32 GetLaps() const { return Laps; }
};
