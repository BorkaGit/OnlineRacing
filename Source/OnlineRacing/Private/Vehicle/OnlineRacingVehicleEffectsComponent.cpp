// Fill out your copyright notice in the Description page of Project Settings.


#include "Vehicle/OnlineRacingVehicleEffectsComponent.h"

#include "Components/SkeletalMeshComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"

#include "OnlineRacing.h"
#include "OnlineRacingPawn.h"
#include "Vehicle/OnlineRacingVehicleTelemetryComponent.h"

namespace
{
	const FName TireSmokeSpawnRateParameterName(TEXT("User.SpawnRate"));
}

UOnlineRacingVehicleEffectsComponent::UOnlineRacingVehicleEffectsComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
}


void UOnlineRacingVehicleEffectsComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!IsValid(TireSmokeSystem))
	{
		UE_LOG(LogOnlineRacing, Warning, TEXT("[VehicleEffects] Tire smoke system is not configured on %s."), *GetNameSafe(GetOwner()));

		SetComponentTickEnabled(false);
		return;
	}

	const AOnlineRacingPawn* const VehiclePawn = Cast<AOnlineRacingPawn>(GetOwner());
	if (!IsValid(VehiclePawn))
	{
		SetComponentTickEnabled(false);
		return;
	}

	VehicleTelemetry = VehiclePawn->GetVehicleTelemetry();
	if (!VehicleTelemetry.IsValid())
	{
		SetComponentTickEnabled(false);
		return;
	}

	AddTickPrerequisiteComponent(VehicleTelemetry.Get());

	USkeletalMeshComponent* const VehicleMesh = VehiclePawn->GetMesh();
	if (!IsValid(VehicleMesh))
	{
		SetComponentTickEnabled(false);
		return;
	}

	RearLeftTireSmoke = CreateTireSmokeComponent(*VehicleMesh, RearLeftSmokeSocketName);

	RearRightTireSmoke = CreateTireSmokeComponent(*VehicleMesh, RearRightSmokeSocketName);

	if (!IsValid(RearLeftTireSmoke) && !IsValid(RearRightTireSmoke))
	{
		SetComponentTickEnabled(false);
	}
}

void UOnlineRacingVehicleEffectsComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (IsValid(RearLeftTireSmoke))
	{
		RearLeftTireSmoke->DeactivateImmediate();
		RearLeftTireSmoke->DestroyComponent();
		RearLeftTireSmoke = nullptr;
	}

	if (IsValid(RearRightTireSmoke))
	{
		RearRightTireSmoke->DeactivateImmediate();
		RearRightTireSmoke->DestroyComponent();
		RearRightTireSmoke = nullptr;
	}

	VehicleTelemetry.Reset();
	RearLeftSmoothedSmokeLevel = 0.f;
	RearRightSmoothedSmokeLevel = 0.f;

	Super::EndPlay(EndPlayReason);
}

void UOnlineRacingVehicleEffectsComponent::UpdateTireSmoke(float DeltaTime)
{
    if (!VehicleTelemetry.IsValid())
    {
	    return;
    }

    UpdateWheelSmoke(DeltaTime, RearLeftWheelIndex, RearLeftTireSmoke, RearLeftSmoothedSmokeLevel);

    UpdateWheelSmoke(DeltaTime, RearRightWheelIndex, RearRightTireSmoke, RearRightSmoothedSmokeLevel);
}

void UOnlineRacingVehicleEffectsComponent::UpdateWheelSmoke(
	const float DeltaTime,
	const int32 WheelIndex,
	UNiagaraComponent* TireSmoke,
	float& SmoothedSmokeLevel) const
{
	if (!IsValid(TireSmoke))
	{
		return;
	}

	bool bInContact = false;
	float SlipMagnitude = 0.f;
	float SkidMagnitude = 0.f;

	if (!VehicleTelemetry->GetWheelTelemetry(WheelIndex, bInContact, SlipMagnitude, SkidMagnitude))
	{
		SmoothedSmokeLevel = 0.f;
		TireSmoke->SetVariableFloat(TireSmokeSpawnRateParameterName, 0.f);
		return;
	}

	if (!bInContact)
	{
		SmoothedSmokeLevel = 0.f;

		TireSmoke->SetVariableFloat(TireSmokeSpawnRateParameterName, 0.f);

		return;
	}

	float SkidLevel = 0.f;

	if (VehicleTelemetry->GetSpeedKmh() >= MinimumSmokeSpeedKmh)
	{
		SkidLevel = FMath::GetMappedRangeValueClamped(
			FVector2D(SkidMagnitudeAtZeroSmoke, SkidMagnitudeAtFullSmoke),
			FVector2D(0.0, 1.0),
			SkidMagnitude);
	}

	const float SlipLevel = FMath::GetMappedRangeValueClamped(
		FVector2D(SlipMagnitudeAtZeroSmoke, SlipMagnitudeAtFullSmoke),
		FVector2D(0.0, 1.0),
		SlipMagnitude);

	const float TargetSmokeLevel = FMath::Max(SkidLevel, SlipLevel);

	float InterpolationSpeed = SmokeFallInterpolationSpeed;
	if (TargetSmokeLevel > SmoothedSmokeLevel)
	{
		InterpolationSpeed = SmokeRiseInterpolationSpeed;
	}

	SmoothedSmokeLevel = FMath::FInterpTo(SmoothedSmokeLevel, TargetSmokeLevel, DeltaTime, InterpolationSpeed);

	float SpawnRate = 0.f;

	if (SmoothedSmokeLevel > KINDA_SMALL_NUMBER)
	{
		const float MinimumSpawnRate = FMath::Min(MinimumActiveSmokeSpawnRate, MaximumSmokeSpawnRate);

		SpawnRate = FMath::Lerp(MinimumSpawnRate, MaximumSmokeSpawnRate, SmoothedSmokeLevel);
	}

	TireSmoke->SetVariableFloat(TireSmokeSpawnRateParameterName, SpawnRate);
}

UNiagaraComponent* UOnlineRacingVehicleEffectsComponent::CreateTireSmokeComponent(USkeletalMeshComponent& VehicleMesh, FName SocketName)
{
	if (!VehicleMesh.DoesSocketExist(SocketName))
	{
		UE_LOG(LogOnlineRacing, Warning, TEXT("[VehicleEffects] Socket %s was not found on %s."), *SocketName.ToString(), *GetNameSafe(&VehicleMesh));

		return nullptr;
	}

	UNiagaraComponent* const TireSmoke = UNiagaraFunctionLibrary::SpawnSystemAttached(
		TireSmokeSystem,
		&VehicleMesh,
		SocketName,
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		EAttachLocation::SnapToTarget,
		false,
		false,
		ENCPoolMethod::None,
		false);

	if (!IsValid(TireSmoke))
	{
		return nullptr;
	}

	TireSmoke->SetVariableFloat(TireSmokeSpawnRateParameterName, 0.f);

	TireSmoke->Activate();

	return TireSmoke;
}


void UOnlineRacingVehicleEffectsComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateTireSmoke(DeltaTime);
}
