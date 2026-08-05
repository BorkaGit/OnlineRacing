// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "OnlineRacingPlayerController.generated.h"

class AOnlineRacingPawn;
class UInputMappingContext;
class UOnlineRacingUI;
class UUserWidget;

UCLASS(Abstract, Config = "Game")
class AOnlineRacingPlayerController : public APlayerController
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

	UPROPERTY(Transient)
	TObjectPtr<AOnlineRacingPawn> VehiclePawn;

	UPROPERTY(EditDefaultsOnly, Category = "Vehicle|UI")
	TSubclassOf<UOnlineRacingUI> VehicleUIClass;

	UPROPERTY(Transient)
	TObjectPtr<UOnlineRacingUI> VehicleUI;

public:
	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnRep_Pawn() override;
	virtual void SetupInputComponent() override;

private:
	UFUNCTION()
	void OnPawnDestroyed(AActor* DestroyedPawn);

	void CacheVehiclePawn();
	bool ShouldUseTouchControls() const;
};
