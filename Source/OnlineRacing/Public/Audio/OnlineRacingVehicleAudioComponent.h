// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/ActorComponent.h"
#include "OnlineRacingVehicleAudioComponent.generated.h"

class UPrimitiveComponent;
class UAudioComponent;
class USoundBase;
class USynthComponentMoto;
class UOnlineRacingVehicleTelemetryComponent;
struct FHitResult;

UCLASS(ClassGroup=(Audio), meta=(BlueprintSpawnableComponent))
class ONLINERACING_API UOnlineRacingVehicleAudioComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UOnlineRacingVehicleAudioComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vehicle|Audio|Transmission")
	TObjectPtr<USoundBase> GearShiftSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vehicle|Audio|Engine")
	TObjectPtr<USoundBase> VehicleLoopSound;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Vehicle|Audio|Engine", meta = (ForceUnits = "km/h"))
	double IdleFadeOutSpeedKmh = 113.0;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Vehicle|Audio|Engine", meta = (ForceUnits = "km/h"))
	double IdleMaximumPitchSpeedKmh = 40.0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vehicle|Audio|Engine", meta = (ClampMin = "0.01"))
	float RpmRiseInterpolationSpeed = 8.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vehicle|Audio|Engine", meta = (ClampMin = "0.01"))
	float RpmFallInterpolationSpeed = 12.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vehicle|Audio|Engine", meta = (ClampMin = "0.01", ClampMax = "1.0"))
	float MotoSynthFullVolumeNormalizedRpm = 0.15f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vehicle|Audio|Engine", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MotoSynthBaseVolumeMultiplier = 0.4f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vehicle|Audio|Engine Load", meta = (ClampMin = "0.01"))
	float LoadFallInterpolationSpeed = 4.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vehicle|Audio|Engine Load", meta = (ClampMin = "0.01"))
	float LoadRiseInterpolationSpeed = 8.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vehicle|Audio|Engine Load", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MotoSynthCoastVolumeMultiplier = 0.7f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vehicle|Audio|Engine Load", meta = (ClampMin = "20.0", ClampMax = "20000.0", Units = "Hz"))
	float MotoSynthCoastLowPassFrequency = 8000.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vehicle|Audio|Engine Load", meta = (ClampMin = "20.0", ClampMax = "20000.0", Units = "Hz"))
	float MotoSynthLoadedLowPassFrequency = 18000.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vehicle|Audio|Tires", meta = (ClampMin = "0.0", ForceUnits = "km/h"))
	float TireRollFadeInEndSpeedKmh = 25.0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vehicle|Audio|Tires", meta = (ClampMin = "0.0", ForceUnits = "km/h"))
	float TireRollFadeOutEndSpeedKmh = 190.0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vehicle|Audio|Tires", meta = (ClampMin = "0.01"))
	float TireRollInterpolationSpeed = 6.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vehicle|Audio|Tires", meta = (ClampMin = "0.0", ForceUnits = "km/h"))
	float MinimumSkidSpeedKmh = 20.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vehicle|Audio|Tires", meta = (ClampMin = "0.01"))
	float SkidRiseInterpolationSpeed = 10.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vehicle|Audio|Tires", meta = (ClampMin = "0.01"))
	float SkidFallInterpolationSpeed = 6.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vehicle|Audio|Tires", meta = (ClampMin = "0.0", ForceUnits = "cm/s"))
	float SkidMagnitudeAtZeroVolume = 300.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vehicle|Audio|Tires", meta = (ClampMin = "0.0", ForceUnits = "cm/s"))
	float SkidMagnitudeAtFullVolume = 900.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vehicle|Audio|Tires", meta = (ClampMin = "0.0", ForceUnits = "cm/s"))
	float SlipMagnitudeAtZeroVolume = 500.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vehicle|Audio|Tires", meta = (ClampMin = "0.0", ForceUnits = "cm/s"))
	float SlipMagnitudeAtFullVolume = 2200.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vehicle|Audio|Wind", meta = (ClampMin = "0.0", ForceUnits = "km/h"))
	float WindFadeInStartSpeedKmh = 60.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vehicle|Audio|Wind", meta = (ClampMin = "0.0", ForceUnits = "km/h"))
	float WindFullVolumeSpeedKmh = 180.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vehicle|Audio|Wind", meta = (ClampMin = "0.01"))
	float WindInterpolationSpeed = 4.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vehicle|Audio|Impacts")
	TObjectPtr<USoundBase> VehicleImpactSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vehicle|Audio|Impacts", meta = (ClampMin = "1.0"))
	float MinimumImpactImpulse = 70000.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vehicle|Audio|Impacts", meta = (ClampMin = "1.0"))
	float FullImpactImpulse = 9000000.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vehicle|Audio|Impacts", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MinimumImpactVolume = 0.15f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vehicle|Audio|Impacts", meta = (ClampMin = "0.0", Units = "s"))
	float ImpactCooldownSeconds = 0.12f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Vehicle|Audio|Debug")
	float SmoothedWindLevel = 0.f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Vehicle|Audio|Debug")
	float SmoothedDriftingLevel = 0.f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Vehicle|Audio|Debug")
	float SmoothedHandbrakeLevel = 0.f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Vehicle|Audio|Debug")
	float SmoothedTireRollLevel = 0.f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Vehicle|Audio|Debug")
	float CurrentMotoSynthLowPassFrequency = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Transient, Category = "Vehicle|Audio|Debug")
	float SmoothedEngineLoad = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Transient, Category = "Vehicle|Audio|Debug")
	float SmoothedEngineRpm = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Transient, Category = "Vehicle|Audio|Debug")
	float CurrentMotoSynthRpm = 0.f;

private:

	void UpdateGearAudio();
	void PlayGearShiftSound();
	void UpdateMotoSynthVolume() const;
	void UpdateMotoSynthFilter();
	void UpdateIdleAudio();
	void UpdateEngineLoad(float DeltaTime);
	void UpdateTireRolling(float DeltaTime);
	void UpdateSkidAudio(float DeltaTime);
	void UpdateWindAudio(float DeltaTime);

	UFUNCTION()
	void HandleVehicleHit(
		UPrimitiveComponent* HitComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		FVector NormalImpulse,
		const FHitResult& Hit);

	TWeakObjectPtr<UOnlineRacingVehicleTelemetryComponent> VehicleTelemetry;
	TWeakObjectPtr<USynthComponentMoto> EngineSynth;
	TWeakObjectPtr<UPrimitiveComponent> CollisionComponent;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> VehicleLoopAudio;

	float MotoSynthMinRpm = 0.f;
	float MotoSynthMaxRpm = 0.f;

	int32 PreviousTargetGear = 0;


	double LastImpactTimeSeconds = -1.0;
};