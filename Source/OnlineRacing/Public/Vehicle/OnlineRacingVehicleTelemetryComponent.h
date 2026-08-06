// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/ActorComponent.h"
#include "OnlineRacingVehicleTelemetryComponent.generated.h"

class UChaosWheeledVehicleMovementComponent;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ONLINERACING_API UOnlineRacingVehicleTelemetryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UOnlineRacingVehicleTelemetryComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintPure, Category = "Vehicle|Telemetry")
	float GetSpeedKmh() const { return SpeedKmh; }

	UFUNCTION(BlueprintPure, Category = "Vehicle|Telemetry")
	float GetEngineRpm() const { return EngineRpm; }

	UFUNCTION(BlueprintPure, Category = "Vehicle|Telemetry")
	float GetNormalizedRpm() const { return NormalizedRpm; }

	UFUNCTION(BlueprintPure, Category = "Vehicle|Telemetry")
	int32 GetCurrentGear() const { return CurrentGear; }

	UFUNCTION(BlueprintPure, Category = "Vehicle|Telemetry")
	float GetThrottleInput() const { return ThrottleInput; }

	UFUNCTION(BlueprintPure, Category = "Vehicle|Telemetry")
	float GetBrakeInput() const { return BrakeInput; }

	UFUNCTION(BlueprintPure, Category = "Vehicle|Telemetry")
	float GetSteeringInput() const { return SteeringInput; }

	UFUNCTION(BlueprintPure, Category = "Vehicle|Telemetry")
	bool IsHandbrakeActive() const { return bHandbrakeActive; }

	UFUNCTION(BlueprintPure, Category = "Vehicle|Telemetry")
	int32 GetWheelsInContact() const { return WheelsInContact; }

	UFUNCTION(BlueprintPure, Category = "Vehicle|Telemetry")
	bool IsAnyWheelSlipping() const { return bAnyWheelSlipping; }

	UFUNCTION(BlueprintPure, Category = "Vehicle|Telemetry")
	bool IsAnyWheelSkidding() const { return bAnyWheelSkidding; }

	UFUNCTION(BlueprintPure, Category = "Vehicle|Telemetry")
	float GetMaxSlipMagnitude() const { return MaxSlipMagnitude; }

	UFUNCTION(BlueprintPure, Category = "Vehicle|Telemetry")
	float GetMaxSkidMagnitude() const { return MaxSkidMagnitude; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Vehicle|Telemetry", meta = (ForceUnits = "km/h"))
	float SpeedKmh = 0.f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Vehicle|Telemetry")
	float EngineRpm = 0.f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Vehicle|Telemetry")
	float NormalizedRpm = 0.f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Vehicle|Telemetry")
	int32 CurrentGear = 0;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Vehicle|Telemetry")
	float ThrottleInput = 0.f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Vehicle|Telemetry")
	float BrakeInput = 0.f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Vehicle|Telemetry")
	float SteeringInput = 0.f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Vehicle|Telemetry")
	bool bHandbrakeActive = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Vehicle|Telemetry")
	int32 WheelsInContact = 0;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Vehicle|Telemetry")
	bool bAnyWheelSlipping = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Vehicle|Telemetry")
	bool bAnyWheelSkidding = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Vehicle|Telemetry", meta = (ForceUnits = "cm/s"))
	float MaxSlipMagnitude = 0.f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Vehicle|Telemetry", meta = (ForceUnits = "cm/s"))
	float MaxSkidMagnitude = 0.f;

	TWeakObjectPtr<UChaosWheeledVehicleMovementComponent> VehicleMovement;

private:
	void UpdateWheelTelemetry();
};
