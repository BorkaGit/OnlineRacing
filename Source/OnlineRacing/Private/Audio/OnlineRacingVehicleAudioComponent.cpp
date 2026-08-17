// Fill out your copyright notice in the Description page of Project Settings.


#include "Audio/OnlineRacingVehicleAudioComponent.h"
#include "GameFramework/Actor.h"
#include "OnlineRacing.h"
#include "Kismet/GameplayStatics.h"
#include "SynthComponents/SynthComponentMoto.h"
#include "Vehicle/OnlineRacingVehicleTelemetryComponent.h"
#include "Components/AudioComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "DSP/Dsp.h"

namespace
{
	const FName RoadNoiseAndWindParameterName(TEXT("RoadNoiseAndWind"));
	const FName DriftingParameterName(TEXT("Drifting"));
	const FName HandbrakeParameterName(TEXT("Handbrake"));
	const FName TireRollParameterName(TEXT("TireRoll"));
	const FName IdleParameterName(TEXT("Idle"));
	const FName IdlePitchParameterName(TEXT("Idle Pitch"));
}

UOnlineRacingVehicleAudioComponent::UOnlineRacingVehicleAudioComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
}


void UOnlineRacingVehicleAudioComponent::BeginPlay()
{
	Super::BeginPlay();

	const AActor* OwnerActor = GetOwner();
	if (!IsValid(OwnerActor))
	{
		return;
	}

	UPrimitiveComponent* const RootPrimitive = Cast<UPrimitiveComponent>(OwnerActor->GetRootComponent());
	if (IsValid(RootPrimitive))
	{
		CollisionComponent = RootPrimitive;

		CollisionComponent->SetNotifyRigidBodyCollision(true);

		CollisionComponent->OnComponentHit.AddUniqueDynamic(this, &ThisClass::HandleVehicleHit);
	}
	else
	{
		UE_LOG(LogOnlineRacing, Error, TEXT("[VehicleAudio] Root component is missing on %s."), *GetNameSafe(OwnerActor));
	}

	VehicleTelemetry = OwnerActor->FindComponentByClass<UOnlineRacingVehicleTelemetryComponent>();
	if (!VehicleTelemetry.IsValid())
	{
		UE_LOG(LogOnlineRacing, Error, TEXT("[VehicleAudio] Telemetry component is missing on %s."), *GetNameSafe(OwnerActor));

		SetComponentTickEnabled(false);
		return;
	}

	AddTickPrerequisiteComponent(VehicleTelemetry.Get());

	EngineSynth = OwnerActor->FindComponentByClass<USynthComponentMoto>();
	if (!EngineSynth.IsValid())
	{
		UE_LOG(LogOnlineRacing, Error, TEXT("[VehicleAudio] Engine synth component is missing on %s."), *GetNameSafe(OwnerActor));

		SetComponentTickEnabled(false);
		return;
	}

	if (!IsValid(EngineSynth->MotoSynthPreset))
	{
		UE_LOG(LogOnlineRacing, Error, TEXT("[VehicleAudio] MotoSynth preset is missing on %s."), *GetNameSafe(OwnerActor));

		SetComponentTickEnabled(false);
		return;
	}

	EngineSynth->GetRPMRange(MotoSynthMinRpm, MotoSynthMaxRpm);
	if (MotoSynthMaxRpm <= MotoSynthMinRpm)
	{
		UE_LOG(LogOnlineRacing, Error, TEXT("[VehicleAudio] MotoSynth max RPM is invalid on %s."), *GetNameSafe(OwnerActor));

		SetComponentTickEnabled(false);
		return;
	}

	const float EngineIdleRpm = VehicleTelemetry->GetEngineIdleRpm();
	const float EngineMaxRpm = VehicleTelemetry->GetEngineMaxRpm();

	if (EngineIdleRpm <= 0.f || EngineMaxRpm <= EngineIdleRpm)
	{
		UE_LOG(LogOnlineRacing, Error, TEXT("[VehicleAudio] Invalid engine RPM range %.0f-%.0f on %s."), EngineIdleRpm, EngineMaxRpm, *GetNameSafe(OwnerActor));

		SetComponentTickEnabled(false);
		return;
	}

	SmoothedEngineRpm = EngineIdleRpm;
	CurrentMotoSynthRpm = MotoSynthMinRpm;

	PreviousTargetGear = VehicleTelemetry->GetTargetGear();

	if (IsValid(VehicleLoopSound))
	{
		USceneComponent* const AttachComponent = OwnerActor->GetRootComponent();

		if (IsValid(AttachComponent))
		{
			VehicleLoopAudio = UGameplayStatics::SpawnSoundAttached(
				VehicleLoopSound,
				AttachComponent,
				NAME_None,
				FVector::ZeroVector,
				FRotator::ZeroRotator,
				EAttachLocation::KeepRelativeOffset,
				true,
				1.f,
				1.f,
				0.f,
				nullptr,
				nullptr,
				false);
		}
	}

	if (IsValid(VehicleLoopAudio))
	{
		EngineSynth->SetVolumeMultiplier(0.f);
	}
	else
	{
		EngineSynth->SetVolumeMultiplier(1.f);

		UE_LOG(LogOnlineRacing, Warning, TEXT("[VehicleAudio] Idle engine sound is missing on %s."), *GetNameSafe(OwnerActor));
	}

	EngineSynth->SetLowPassFilterEnabled(true);

	EngineSynth->Start();

	UE_LOG(
	LogOnlineRacing,
	Display,
	TEXT("[VehicleAudio] Started on %s. Engine RPM: %.0f-%.0f, MotoSynth RPM: %.0f-%.0f."),
	*GetNameSafe(OwnerActor),
	EngineIdleRpm,
	EngineMaxRpm,
	MotoSynthMinRpm,
	MotoSynthMaxRpm);
}

void UOnlineRacingVehicleAudioComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (CollisionComponent.IsValid())
	{
		CollisionComponent->OnComponentHit.RemoveDynamic(this, &ThisClass::HandleVehicleHit);

		CollisionComponent.Reset();
	}

	if (IsValid(VehicleLoopAudio))
	{
		VehicleLoopAudio->Stop();
		VehicleLoopAudio->DestroyComponent();
		VehicleLoopAudio = nullptr;
	}

	if (EngineSynth.IsValid())
	{
		EngineSynth->Stop();
	}

	Super::EndPlay(EndPlayReason);
}

void UOnlineRacingVehicleAudioComponent::UpdateGearAudio()
{
	const int32 TargetGear = VehicleTelemetry->GetTargetGear();

	if (PreviousTargetGear > 0 && TargetGear > PreviousTargetGear)
	{
		PlayGearShiftSound();
	}

	PreviousTargetGear = TargetGear;
}

void UOnlineRacingVehicleAudioComponent::PlayGearShiftSound()
{
	if (!IsValid(GearShiftSound))
	{
		return;
	}

	const AActor* OwnerActor = GetOwner();
	if (!IsValid(OwnerActor))
	{
		return;
	}

	USceneComponent* const AttachComponent = OwnerActor->GetRootComponent();
	if (!IsValid(AttachComponent))
	{
		return;
	}

	UGameplayStatics::SpawnSoundAttached(GearShiftSound, AttachComponent, NAME_None, FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset, true);
}

void UOnlineRacingVehicleAudioComponent::UpdateMotoSynthVolume() const
{
	float MotoSynthVolume = 1.f;

	if (IsValid(VehicleLoopAudio))
	{
		const float FadeEndRpm = FMath::Lerp(MotoSynthMinRpm, MotoSynthMaxRpm, MotoSynthFullVolumeNormalizedRpm);

		MotoSynthVolume = FMath::GetMappedRangeValueClamped(FVector2D(MotoSynthMinRpm, FadeEndRpm), FVector2D(0.0, 1.0), CurrentMotoSynthRpm);

	}

	const float LoadVolumeMultiplier = FMath::Lerp(MotoSynthCoastVolumeMultiplier, 1.f, SmoothedEngineLoad);

	MotoSynthVolume *= LoadVolumeMultiplier;
	MotoSynthVolume *= MotoSynthBaseVolumeMultiplier;

	EngineSynth->SetVolumeMultiplier(MotoSynthVolume);
}

void UOnlineRacingVehicleAudioComponent::UpdateMotoSynthFilter()
{
	CurrentMotoSynthLowPassFrequency = Audio::GetLogFrequencyClamped(SmoothedEngineLoad, FVector2D(0.0, 1.0),
		FVector2D(
		MotoSynthCoastLowPassFrequency,
		MotoSynthLoadedLowPassFrequency));

	EngineSynth->SetLowPassFilterFrequency(CurrentMotoSynthLowPassFrequency);
}

void UOnlineRacingVehicleAudioComponent::UpdateIdleAudio()
{
	if (!IsValid(VehicleLoopAudio))
	{
		return;
	}

	const float IdleLevel = FMath::GetMappedRangeValueClamped(FVector2D(0.0, IdleFadeOutSpeedKmh), FVector2D(1.0, 0.0), VehicleTelemetry->GetSpeedKmh());

	const float IdlePitch = FMath::GetMappedRangeValueClamped(FVector2D(0.0, IdleMaximumPitchSpeedKmh), FVector2D(0.0, 3.0), VehicleTelemetry->GetSpeedKmh());

	VehicleLoopAudio->SetFloatParameter(IdleParameterName, IdleLevel);
	VehicleLoopAudio->SetFloatParameter(IdlePitchParameterName, IdlePitch);
}

void UOnlineRacingVehicleAudioComponent::UpdateEngineLoad(float DeltaTime)
{
	const float TargetLoad = FMath::Clamp(FMath::Abs(VehicleTelemetry->GetThrottleInput()), 0.f, 1.f);

	float InterpolationSpeed = LoadFallInterpolationSpeed;
	if (TargetLoad > SmoothedEngineLoad)
	{
		InterpolationSpeed = LoadRiseInterpolationSpeed;
	}

	SmoothedEngineLoad = FMath::FInterpTo(SmoothedEngineLoad, TargetLoad, DeltaTime, InterpolationSpeed);
}

void UOnlineRacingVehicleAudioComponent::UpdateTireRolling(float DeltaTime)
{
	if (!VehicleTelemetry.IsValid() || !IsValid(VehicleLoopAudio))
	{
		return;
	}

	const float SpeedKmh = VehicleTelemetry->GetSpeedKmh();

	const float LowSpeedFade = FMath::GetMappedRangeValueClamped(
		FVector2D(0.0, TireRollFadeInEndSpeedKmh),
		FVector2D(0.0, 1.0),
		SpeedKmh);

	const float HighSpeedFade = FMath::GetMappedRangeValueClamped(
		FVector2D(TireRollFadeInEndSpeedKmh, TireRollFadeOutEndSpeedKmh),
		FVector2D(1.0, 0.0),
		SpeedKmh);

	const float AccelerationMultiplier = FMath::Lerp(1.f, HighSpeedFade, SmoothedEngineLoad);

	float TargetTireRollLevel = LowSpeedFade * AccelerationMultiplier;

	if (VehicleTelemetry->GetWheelsInContact() <= 0)
	{
		TargetTireRollLevel = 0.f;
	}

	SmoothedTireRollLevel = FMath::FInterpTo(SmoothedTireRollLevel, TargetTireRollLevel, DeltaTime, TireRollInterpolationSpeed);

	VehicleLoopAudio->SetFloatParameter(TireRollParameterName, SmoothedTireRollLevel);
}

void UOnlineRacingVehicleAudioComponent::UpdateSkidAudio(float DeltaTime)
{
	if (!VehicleTelemetry.IsValid() || !IsValid(VehicleLoopAudio))
	{
		return;
	}

	float TargetDriftingLevel = 0.f;
	float TargetHandbrakeLevel = 0.f;

	const bool bHasWheelContact = VehicleTelemetry->GetWheelsInContact() > 0;

	const bool bHasEnoughSpeed = VehicleTelemetry->GetSpeedKmh() >= MinimumSkidSpeedKmh;

	if (bHasWheelContact && bHasEnoughSpeed)
	{
		float SkidLevel = 0.f;
		if (VehicleTelemetry->IsAnyWheelSkidding())
		{
			SkidLevel = FMath::GetMappedRangeValueClamped(
				FVector2D(SkidMagnitudeAtZeroVolume, SkidMagnitudeAtFullVolume),
				FVector2D(0.0, 1.0),
				VehicleTelemetry->GetMaxSkidMagnitude());
		}

		float SlipLevel = 0.f;
		if (VehicleTelemetry->IsAnyWheelSlipping())
		{
			SlipLevel = FMath::GetMappedRangeValueClamped(
				FVector2D(SlipMagnitudeAtZeroVolume, SlipMagnitudeAtFullVolume),
				FVector2D(0.0, 1.0),
				VehicleTelemetry->GetMaxSlipMagnitude());
		}

		const float TireSkidLevel = FMath::Max(SkidLevel, SlipLevel);

		if (VehicleTelemetry->IsHandbrakeActive())
		{
			TargetHandbrakeLevel = TireSkidLevel;
		}
		else
		{
			TargetDriftingLevel = TireSkidLevel;
		}
	}

	float DriftingInterpolationSpeed = SkidFallInterpolationSpeed;
	if (TargetDriftingLevel > SmoothedDriftingLevel)
	{
		DriftingInterpolationSpeed = SkidRiseInterpolationSpeed;
	}

	float HandbrakeInterpolationSpeed = SkidFallInterpolationSpeed;
	if (TargetHandbrakeLevel > SmoothedHandbrakeLevel)
	{
		HandbrakeInterpolationSpeed = SkidRiseInterpolationSpeed;
	}

	SmoothedDriftingLevel = FMath::FInterpTo(SmoothedDriftingLevel, TargetDriftingLevel, DeltaTime, DriftingInterpolationSpeed);

	SmoothedHandbrakeLevel = FMath::FInterpTo(SmoothedHandbrakeLevel, TargetHandbrakeLevel, DeltaTime, HandbrakeInterpolationSpeed);

	VehicleLoopAudio->SetFloatParameter(DriftingParameterName, SmoothedDriftingLevel);
	VehicleLoopAudio->SetFloatParameter(HandbrakeParameterName, SmoothedHandbrakeLevel);
}

void UOnlineRacingVehicleAudioComponent::UpdateWindAudio(float DeltaTime)
{
	if (!VehicleTelemetry.IsValid() || !IsValid(VehicleLoopAudio))
	{
		return;
	}

	const float TargetWindLevel = FMath::GetMappedRangeValueClamped(
		FVector2D(WindFadeInStartSpeedKmh, WindFullVolumeSpeedKmh),
		FVector2d(0.0, 1.0),
		VehicleTelemetry->GetSpeedKmh());

	SmoothedWindLevel = FMath::FInterpTo(
		SmoothedWindLevel,
		TargetWindLevel,
		DeltaTime,
		WindInterpolationSpeed);

	VehicleLoopAudio->SetFloatParameter(RoadNoiseAndWindParameterName, SmoothedWindLevel);
}

void UOnlineRacingVehicleAudioComponent::HandleVehicleHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (!IsValid(VehicleImpactSound))
	{
		return;
	}

	const float ImpactImpulse = NormalImpulse.Size();
	if (ImpactImpulse < MinimumImpactImpulse)
	{
		return;
	}

	const UWorld* const CurrentWorld = GetWorld();
	if (!IsValid(CurrentWorld))
	{
		return;
	}

	const double CurrentTimeSeconds = CurrentWorld->GetTimeSeconds();

	if (LastImpactTimeSeconds >= 0.0 && CurrentTimeSeconds - LastImpactTimeSeconds < ImpactCooldownSeconds)
	{
		return;
	}

	LastImpactTimeSeconds = CurrentTimeSeconds;

	const float ClampedImpulse = FMath::Min(ImpactImpulse, FullImpactImpulse);

	const float SafeFullImpactImpulse = FMath::Max(FullImpactImpulse, MinimumImpactImpulse + 1.f);

	const float ImpactAlpha = FMath::GetMappedRangeValueClamped(
		FVector2D(FMath::Loge(MinimumImpactImpulse),FMath::Loge(SafeFullImpactImpulse)),
		FVector2D(0.0, 1.0),
		FMath::Loge(ClampedImpulse));

	const float VolumeMultiplier = FMath::Lerp(MinimumImpactVolume, 1.f, ImpactAlpha);

	UGameplayStatics::PlaySoundAtLocation(this, VehicleImpactSound, Hit.ImpactPoint, VolumeMultiplier);
}

void UOnlineRacingVehicleAudioComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!VehicleTelemetry.IsValid() || !EngineSynth.IsValid())
	{
		return;
	}

	UpdateEngineLoad(DeltaTime);
	UpdateTireRolling(DeltaTime);
	UpdateSkidAudio(DeltaTime);
	UpdateWindAudio(DeltaTime);
	UpdateMotoSynthFilter();
	UpdateGearAudio();

	const float EngineIdleRpm = VehicleTelemetry->GetEngineIdleRpm();
	const float EngineMaxRpm = VehicleTelemetry->GetEngineMaxRpm();

	const float TargetEngineRpm = FMath::Clamp(VehicleTelemetry->GetEngineRpm(), EngineIdleRpm, EngineMaxRpm);

	float InterpolationSpeed = RpmFallInterpolationSpeed;
	if (TargetEngineRpm > SmoothedEngineRpm)
	{
		InterpolationSpeed = RpmRiseInterpolationSpeed;
	}

	SmoothedEngineRpm = FMath::FInterpTo(SmoothedEngineRpm, TargetEngineRpm, DeltaTime, InterpolationSpeed);

	CurrentMotoSynthRpm = Audio::GetLogFrequencyClamped(
	SmoothedEngineRpm,
	FVector2D(EngineIdleRpm, EngineMaxRpm),
	FVector2D(MotoSynthMinRpm, MotoSynthMaxRpm));

	EngineSynth->SetRPM(CurrentMotoSynthRpm, DeltaTime);

	UpdateIdleAudio();
	UpdateMotoSynthVolume();
}
