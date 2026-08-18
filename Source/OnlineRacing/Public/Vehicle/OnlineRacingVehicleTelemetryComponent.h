// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Chaos/ChaosEngineInterface.h"
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

	FORCEINLINE float GetSpeedKmh() const { return SpeedKmh; }

	FORCEINLINE float GetEngineIdleRpm() const { return EngineIdleRpm; }

	FORCEINLINE float GetEngineMaxRpm() const { return EngineMaxRpm; }

	FORCEINLINE float GetEngineRpm() const { return EngineRpm; }

	FORCEINLINE float GetNormalizedRpm() const { return NormalizedRpm; }

	FORCEINLINE int32 GetCurrentGear() const { return CurrentGear; }

	FORCEINLINE int32 GetTargetGear() const { return TargetGear; }

	FORCEINLINE float GetThrottleInput() const { return ThrottleInput; }

	FORCEINLINE float GetBrakeInput() const { return BrakeInput; }

	FORCEINLINE float GetSteeringInput() const { return SteeringInput; }

	FORCEINLINE bool IsHandbrakeActive() const { return bHandbrakeActive; }

	FORCEINLINE int32 GetWheelsInContact() const { return WheelsInContact; }

	FORCEINLINE bool IsAnyWheelSlipping() const { return bAnyWheelSlipping; }

	FORCEINLINE bool IsAnyWheelSkidding() const { return bAnyWheelSkidding; }

	FORCEINLINE float GetMaxSlipMagnitude() const { return MaxSlipMagnitude; }

	FORCEINLINE float GetMaxSkidMagnitude() const { return MaxSkidMagnitude; }

	FORCEINLINE EPhysicalSurface GetCurrentSurfaceType() const
	{
		return CurrentSurfaceType.GetValue();
	}

	bool GetWheelTelemetry(int32 WheelIndex, bool& bOutInContact, float& OutSlipMagnitude, float& OutSkidMagnitude) const;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Vehicle|Telemetry", meta = (ForceUnits = "km/h"))
	float SpeedKmh = 0.f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Vehicle|Telemetry")
	float EngineIdleRpm = 0.f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Vehicle|Telemetry")
	float EngineMaxRpm = 0.f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Vehicle|Telemetry")
	float EngineRpm = 0.f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Vehicle|Telemetry")
	float NormalizedRpm = 0.f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Vehicle|Telemetry")
	int32 CurrentGear = 0;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Vehicle|Telemetry")
	int32 TargetGear = 0;

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

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Vehicle|Telemetry")
	TEnumAsByte<EPhysicalSurface> CurrentSurfaceType = SurfaceType_Default;

	TWeakObjectPtr<UChaosWheeledVehicleMovementComponent> VehicleMovement;

private:
	void UpdateWheelTelemetry();
};
