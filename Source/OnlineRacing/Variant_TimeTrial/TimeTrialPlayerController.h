// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TimeTrialPlayerController.generated.h"

class AOnlineRacingPawn;
class ATimeTrialTrackGate;
class UInputMappingContext;
class UOnlineRacingUI;
class UTimeTrialUI;
class UUserWidget;

// Legacy Vehicle Template example. Production race state will live in RaceGameState and RacePlayerState.
UCLASS(Abstract, Config = "Game")
class ATimeTrialPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Input|Input Mappings")
	TArray<TObjectPtr<UInputMappingContext>> DefaultMappingContexts;

	UPROPERTY(EditDefaultsOnly, Category = "Input|Input Mappings")
	TArray<TObjectPtr<UInputMappingContext>> MobileExcludedMappingContexts;

	UPROPERTY(EditDefaultsOnly, Category = "Input|Touch Controls")
	TSubclassOf<UUserWidget> MobileControlsWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> MobileControlsWidget;

	UPROPERTY(EditDefaultsOnly, Config, Category = "Input|Touch Controls")
	bool bForceTouchControls = false;

	UPROPERTY(EditDefaultsOnly, Category = "Input|Steering Wheel Controls")
	bool bUseSteeringWheelControls = false;

	UPROPERTY(EditDefaultsOnly, Category = "Input|Steering Wheel Controls", meta = (EditCondition = "bUseSteeringWheelControls"))
	TObjectPtr<UInputMappingContext> SteeringWheelInputMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Time Trial|UI")
	TSubclassOf<UTimeTrialUI> UIWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UTimeTrialUI> UIWidget;

	UPROPERTY(EditDefaultsOnly, Category = "Vehicle|UI")
	TSubclassOf<UOnlineRacingUI> VehicleUIClass;

	UPROPERTY(Transient)
	TObjectPtr<UOnlineRacingUI> VehicleUI;

	UPROPERTY(Transient)
	TObjectPtr<ATimeTrialTrackGate> TargetGate;

	UPROPERTY(Transient)
	TObjectPtr<AOnlineRacingPawn> VehiclePawn;

	int32 CurrentLap = 0;
	bool bRaceStarted = false;

public:
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION()
	void StartRace();

	void IncrementLapCount();
	ATimeTrialTrackGate* GetTargetGate() const;
	void SetTargetGate(ATimeTrialTrackGate* Gate);

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnRep_Pawn() override;
	virtual void SetupInputComponent() override;

private:
	UFUNCTION()
	void OnPawnDestroyed(AActor* DestroyedPawn);

	void CacheVehiclePawn();
	bool ShouldUseTouchControls() const;
};
