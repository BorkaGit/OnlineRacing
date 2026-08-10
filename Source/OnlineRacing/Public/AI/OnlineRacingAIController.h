// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AIController.h"
#include "OnlineRacingAIController.generated.h"

class USplineComponent;
class AOnlineRacingPawn;
class AOnlineRacingDrivingLine;

UCLASS()
class ONLINERACING_API AOnlineRacingAIController : public AAIController
{
	GENERATED_BODY()
	
public:
	AOnlineRacingAIController();
	
	virtual void Tick(float DeltaSeconds) override;
	
protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	
	virtual void BeginPlay() override;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Identity")
	FString BotDisplayName = TEXT("AI Racer");
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Driving", meta = (ClampMin = "100.0", ForceUnits = "cm"))
	float LookAheadDistance = 1200.f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Driving", meta = (ClampMin = "0.1", ClampMax = "5.0"))
	float SteeringGain = 1.f;
	
	/**
	 * Desired speed when the racing target is approximately straight ahead.
	 * This is the AI vehicle's maximum normal racing speed.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Speed", meta = (ClampMin = "1.0", ForceUnits = "km/h"))
	float StraightTargetSpeedKmh = 60.f;

	/**
	 * Desired speed when the steering input reaches its maximum magnitude.
	 * Lower values make the AI safer but slower through sharp turns.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Speed", meta = (ClampMin = "1.0", ForceUnits = "km/h"))
	float CornerTargetSpeedKmh = 25.f;

	/**
	 * Speed difference that produces full throttle or full brake.
	 * Larger values produce smoother, weaker speed corrections.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Speed", meta = (ClampMin = "1.0", ForceUnits = "km/h"))
	float SpeedControlRangeKmh = 20.f;

	/**
	 * Allowed overspeed before braking begins.
	 * Prevents rapid switching between coasting and braking near target speed.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Speed", meta = (ClampMin = "0.0", ForceUnits = "km/h"))
	float BrakeDeadZoneKmh = 5.f;
	
	/**
	* Distance to the first spline direction sample.
	*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Corner Prediction", meta = (ClampMin = "0.0", Units = "cm"))
	float NearTurnSampleDistance = 800.f;

	/**
	 * Distance to the second spline direction sample.
	 * Must be greater than NearTurnSampleDistance.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Corner Prediction", meta = (ClampMin = "100.0", Units = "cm"))
	float FarTurnSampleDistance = 3000.f;

	/**
	 * Difference between sampled spline directions that represents a full turn.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Corner Prediction", meta = (ClampMin = "1.0", ClampMax = "180.0", Units = "deg"))
	float FullTurnAngleDegrees = 45.f;
	
	/**
	* Vehicle speed below which the AI may be considered stuck.
	*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Recovery", meta = (ClampMin = "0.0", Units = "km/h"))
	float StuckSpeedThresholdKmh = 3.f;

	/**
	 * Minimum throttle indicating that the AI is actively trying to move.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Recovery", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float StuckThrottleThreshold = 0.2f;

	/**
	 * Time the vehicle must remain stuck before recovery is requested.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Recovery", meta = (ClampMin = "0.1", Units = "s"))
	float StuckTimeThreshold = 4.f;
	
	/**
	* Time during which stuck detection is disabled after a recovery.
	* Allows the vehicle physics to settle and resume movement.
	*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Recovery", meta = (ClampMin = "0.0", Units = "s"))
	float RecoveryCooldown = 2.f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Debug")
	bool bDrawDrivingDebug = true;
	
private:
	
	bool CanDrive() const;
	
	void SetDrivingEnabled(bool bEnabled);
	
	void FindDrivingLine();

	bool UpdateStuckDetection(float DeltaSeconds);
	void RequestRecovery();
	
	static float NormalizeSplineDistance(const USplineComponent& SplineComponent, const float Distance);
	
	bool TryGetDrivingTarget(FVector& OutTargetLocation, float& OutCurrentSplineDistance) const;
	
	float CalculateSteeringInput(const FVector& TargetLocation) const;
	
	float CalculateTargetSpeedKmh(float TurnAmount) const;
	
	float CalculateUpcomingTurnAmount(const float CurrentSplineDistance) const;
	
	bool ApplyDrivingInput(float SteeringInput, float TargetSpeedKmh);
	
	void DrawDrivingDebug(const FVector& TargetLocation, float SteeringInput, float UpcomingTurnAmount) const;
	
	TWeakObjectPtr<AOnlineRacingPawn> VehiclePawn;
	TWeakObjectPtr<AOnlineRacingDrivingLine> DrivingLine;
	
	float StuckTime = 0.f;
	float RecoveryCooldownRemaining = 0.f;
	
	bool bDrivingEnabled = false;
};
