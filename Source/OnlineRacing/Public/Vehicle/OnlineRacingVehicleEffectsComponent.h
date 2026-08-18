// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "OnlineRacingVehicleEffectsComponent.generated.h"

class USkeletalMeshComponent;
class UNiagaraComponent;
class UNiagaraSystem;
class UOnlineRacingVehicleTelemetryComponent;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ONLINERACING_API UOnlineRacingVehicleEffectsComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UOnlineRacingVehicleEffectsComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vehicle|Effects|Tire Smoke")
	int32 RearLeftWheelIndex = 2;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vehicle|Effects|Tire Smoke")
	int32 RearRightWheelIndex = 3;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vehicle|Effects|Tire Smoke")
	TObjectPtr<UNiagaraSystem> TireSmokeSystem;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vehicle|Effects|Tire Smoke")
	FName RearLeftSmokeSocketName = TEXT("TireSmoke_BL");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vehicle|Effects|Tire Smoke")
	FName RearRightSmokeSocketName = TEXT("TireSmoke_BR");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vehicle|Effects|Tire Smoke", meta = (ClampMin = "0.0", ForceUnits = "km/h"))
	float MinimumSmokeSpeedKmh = 20.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vehicle|Effects|Tire Smoke", meta = (ClampMin = "0.0"))
	float SkidMagnitudeAtZeroSmoke = 300.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vehicle|Effects|Tire Smoke", meta = (ClampMin = "0.0"))
	float SkidMagnitudeAtFullSmoke = 900.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vehicle|Effects|Tire Smoke", meta = (ClampMin = "0.0"))
	float SlipMagnitudeAtZeroSmoke = 500.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vehicle|Effects|Tire Smoke", meta = (ClampMin = "0.0"))
	float SlipMagnitudeAtFullSmoke = 2200.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vehicle|Effects|Tire Smoke", meta = (ClampMin = "0.0"))
	float MinimumActiveSmokeSpawnRate = 60.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vehicle|Effects|Tire Smoke", meta = (ClampMin = "0.0"))
	float MaximumSmokeSpawnRate = 160.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vehicle|Effects|Tire Smoke", meta = (ClampMin = "0.0"))
	float SmokeRiseInterpolationSpeed = 8.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Vehicle|Effects|Tire Smoke", meta = (ClampMin = "0.0"))
	float SmokeFallInterpolationSpeed = 4.f;

private:

	void UpdateTireSmoke(float DeltaTime);

	void UpdateWheelSmoke(float DeltaTime, int32 WheelIndex, UNiagaraComponent* TireSmoke, float& SmoothedSmokeLevel) const;

	UNiagaraComponent* CreateTireSmokeComponent(USkeletalMeshComponent& VehicleMesh, FName SocketName);

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Vehicle|Effects|Debug", meta = (AllowPrivateAccess = "true"))
	float RearLeftSmoothedSmokeLevel = 0.f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category = "Vehicle|Effects|Debug", meta = (AllowPrivateAccess = "true"))
	float RearRightSmoothedSmokeLevel = 0.f;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> RearLeftTireSmoke;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> RearRightTireSmoke;

	TWeakObjectPtr<UOnlineRacingVehicleTelemetryComponent> VehicleTelemetry;
};