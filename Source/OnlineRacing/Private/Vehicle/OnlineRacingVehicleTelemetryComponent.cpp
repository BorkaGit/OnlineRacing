// Fill out your copyright notice in the Description page of Project Settings.


#include "Vehicle/OnlineRacingVehicleTelemetryComponent.h"

#include "ChaosWheeledVehicleMovementComponent.h"
#include "GameFramework/Actor.h"

#include "OnlineRacing.h"


UOnlineRacingVehicleTelemetryComponent::UOnlineRacingVehicleTelemetryComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

void UOnlineRacingVehicleTelemetryComponent::BeginPlay()
{
	Super::BeginPlay();

	const AActor* OwnerActor = GetOwner();
	if (!IsValid(OwnerActor))
	{
		return;
	}

	VehicleMovement = OwnerActor->FindComponentByClass<UChaosWheeledVehicleMovementComponent>();
	if (!VehicleMovement.IsValid())
	{
		UE_LOG(LogOnlineRacing, Error, TEXT("[VehicleTelemetry] Chaos movement component is missing on %s."), *GetNameSafe(GetOwner()));

		SetComponentTickEnabled(false);
		return;
	}

	EngineIdleRpm = VehicleMovement->EngineSetup.EngineIdleRPM;
	EngineMaxRpm = VehicleMovement->GetEngineMaxRotationSpeed();
}

void UOnlineRacingVehicleTelemetryComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!VehicleMovement.IsValid())
	{
		return;
	}

	SpeedKmh = FMath::Abs(Chaos::CmSToKmH(VehicleMovement->GetForwardSpeed()));
	EngineRpm = VehicleMovement->GetEngineRotationSpeed();

	const float MaxEngineRpm = VehicleMovement->GetEngineMaxRotationSpeed();
	NormalizedRpm = 0.f;
	if (!FMath::IsNearlyZero(MaxEngineRpm))
	{
		NormalizedRpm = FMath::Clamp(EngineRpm / MaxEngineRpm, 0.f, 1.f);
	}

	CurrentGear = VehicleMovement->GetCurrentGear();
	TargetGear = VehicleMovement->GetTargetGear();
	ThrottleInput = VehicleMovement->GetThrottleInput();
	BrakeInput = VehicleMovement->GetBrakeInput();
	SteeringInput = VehicleMovement->GetSteeringInput();
	bHandbrakeActive = VehicleMovement->GetHandbrakeInput();

	UpdateWheelTelemetry();
}

void UOnlineRacingVehicleTelemetryComponent::UpdateWheelTelemetry()
{
	WheelsInContact = 0;
	bAnyWheelSlipping = false;
	bAnyWheelSkidding = false;
	MaxSlipMagnitude = 0.f;
	MaxSkidMagnitude = 0.f;

	const int32 NumWheels = VehicleMovement->GetNumWheels();
	for (int32 WheelIndex = 0; WheelIndex < NumWheels; ++WheelIndex)
	{
		const FWheelStatus& WheelStatus = VehicleMovement->GetWheelState(WheelIndex);
		if (!WheelStatus.bIsValid || !WheelStatus.bInContact)
		{
			continue;
		}

		++WheelsInContact;

		if (WheelStatus.bIsSlipping)
		{
			bAnyWheelSlipping = true;
		}

		if (WheelStatus.bIsSkidding)
		{
			bAnyWheelSkidding = true;
		}

		MaxSlipMagnitude = FMath::Max(MaxSlipMagnitude, FMath::Abs(WheelStatus.SlipMagnitude));
		MaxSkidMagnitude = FMath::Max(MaxSkidMagnitude, FMath::Abs(WheelStatus.SkidMagnitude));
	}
}
