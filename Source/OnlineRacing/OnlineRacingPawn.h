// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "WheeledVehiclePawn.h"
#include "OnlineRacingPawn.generated.h"

class UOnlineRacingVehicleEffectsComponent;
class USynthComponentMoto;
class UOnlineRacingVehicleAudioComponent;
class UCameraComponent;
class UChaosWheeledVehicleMovementComponent;
class UInputAction;
class USpringArmComponent;
class UOnlineRacingVehicleTelemetryComponent;
struct FInputActionValue;

UCLASS(Abstract)
class AOnlineRacingPawn : public AWheeledVehiclePawn
{
	GENERATED_BODY()

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> FrontSpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> FrontCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> BackSpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> BackCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UOnlineRacingVehicleTelemetryComponent> VehicleTelemetry;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UOnlineRacingVehicleAudioComponent> VehicleAudio;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USynthComponentMoto> EngineSynth;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UOnlineRacingVehicleEffectsComponent> VehicleEffects;

	UPROPERTY(Transient)
	TObjectPtr<UChaosWheeledVehicleMovementComponent> ChaosVehicleMovement;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> SteeringAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> ThrottleAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> BrakeAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> HandbrakeAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> LookAroundAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> ToggleCameraAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> ResetVehicleAction;

	UPROPERTY(EditDefaultsOnly, Category = "Vehicle|Flip Check", meta = (ClampMin = "0.1", Units = "s"))
	float FlipCheckTime = 3.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Vehicle|Flip Check", meta = (ClampMin = "-1.0", ClampMax = "1.0"))
	float FlipCheckMinDot = -0.2f;

	bool bFrontCameraActive = false;
	bool bPreviousFlipCheck = false;
	FTimerHandle FlipCheckTimer;

public:
	AOnlineRacingPawn();

	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category = "Input")
	void DoSteering(float SteeringValue);

	UFUNCTION(BlueprintCallable, Category = "Input")
	void DoThrottle(float ThrottleValue);

	UFUNCTION(BlueprintCallable, Category = "Input")
	void DoBrake(float BrakeValue);

	UFUNCTION(BlueprintCallable, Category = "Input")
	void DoBrakeStart();

	UFUNCTION(BlueprintCallable, Category = "Input")
	void DoBrakeStop();

	UFUNCTION(BlueprintCallable, Category = "Input")
	void DoHandbrakeStart();

	UFUNCTION(BlueprintCallable, Category = "Input")
	void DoHandbrakeStop();

	UFUNCTION(BlueprintCallable, Category = "Input")
	void DoLookAround(float YawDelta);

	UFUNCTION(BlueprintCallable, Category = "Input")
	void DoToggleCamera();

	UFUNCTION(BlueprintCallable, Category = "Input")
	void DoResetVehicle();

	USpringArmComponent* GetFrontSpringArm() const { return FrontSpringArm.Get(); }
	UCameraComponent* GetFollowCamera() const { return FrontCamera.Get(); }
	USpringArmComponent* GetBackSpringArm() const { return BackSpringArm.Get(); }
	UCameraComponent* GetBackCamera() const { return BackCamera.Get(); }
	UChaosWheeledVehicleMovementComponent* GetChaosVehicleMovement() const { return ChaosVehicleMovement.Get(); }
	UOnlineRacingVehicleTelemetryComponent* GetVehicleTelemetry() const { return VehicleTelemetry.Get(); }
	void ResetVehicleAtCurrentLocation();
	void RespawnVehicleAtTransform(const FTransform& RespawnTransform);
	void SetRaceInputEnabled(bool bEnabled);

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "Vehicle")
	void BrakeLights(bool bBraking);

private:
	void Brake(const FInputActionValue& Value);
	void FlippedCheck();
	void HandbrakeStarted(const FInputActionValue& Value);
	void HandbrakeStopped(const FInputActionValue& Value);
	void LookAround(const FInputActionValue& Value);
	void ResetVehicle(const FInputActionValue& Value);
	void Steering(const FInputActionValue& Value);
	void Throttle(const FInputActionValue& Value);
	void ToggleCamera(const FInputActionValue& Value);
	void BrakeStarted(const FInputActionValue& Value);
	void BrakeStopped(const FInputActionValue& Value);

	bool bRaceInputEnabled = true;

	UFUNCTION(Server, Reliable)
	void Server_ResetVehicle();
};
