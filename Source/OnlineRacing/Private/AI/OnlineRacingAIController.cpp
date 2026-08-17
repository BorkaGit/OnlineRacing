// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/OnlineRacingAIController.h"

#include "EngineUtils.h"
#include "OnlineRacing.h"
#include "OnlineRacingPawn.h"
#include "AI/OnlineRacingDrivingLine.h"
#include "Components/SplineComponent.h"
#include "GameFramework/PlayerState.h"
#include "Race/OnlineRacingMatchGameMode.h"
#include "Race/OnlineRacingMatchGameState.h"
#include "Race/OnlineRacingMatchPlayerState.h"
#include "Vehicle/OnlineRacingVehicleTelemetryComponent.h"


AOnlineRacingAIController::AOnlineRacingAIController()
{
	bWantsPlayerState = true;
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;
}

void AOnlineRacingAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	VehiclePawn = Cast<AOnlineRacingPawn>(InPawn);
	if (!VehiclePawn.IsValid())
	{
		UE_LOG(
			LogOnlineRacing,
			Error,
			TEXT("[Server][VehicleAI] %s cannot control incompatible Pawn %s."),
			*GetNameSafe(this),
			*GetNameSafe(InPawn));
		return;
	}

	VehiclePawn->SetRaceInputEnabled(false);
	bDrivingEnabled = false;
	StuckTime = 0.f;
	RecoveryCooldownRemaining = 0.f;

	APlayerState* const BotPlayerState = GetPlayerState<APlayerState>();
	if (IsValid(BotPlayerState))
	{
		BotPlayerState->SetPlayerName(BotDisplayName);
	}

	UE_LOG(
		LogOnlineRacing,
		Display,
		TEXT("[Server][VehicleAI] %s possessed vehicle %s with PlayerState %s."),
		*GetNameSafe(this),
		*GetNameSafe(VehiclePawn.Get()),
		*GetNameSafe(BotPlayerState));
}

void AOnlineRacingAIController::OnUnPossess()
{

	if (VehiclePawn.IsValid())
	{
		VehiclePawn->SetRaceInputEnabled(false);
	}

	bDrivingEnabled = false;
	StuckTime = 0.f;
	RecoveryCooldownRemaining = 0.f;
	VehiclePawn.Reset();

	Super::OnUnPossess();
}

void AOnlineRacingAIController::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority())
	{
		return;
	}

	FindDrivingLine();
}

bool AOnlineRacingAIController::CanDrive() const
{
	const UWorld* CurrentWorld = GetWorld();
	if (!IsValid(CurrentWorld))
	{
		return false;
	}

	if (!VehiclePawn.IsValid())
	{
		return false;
	}

	const AOnlineRacingMatchGameState* const RaceGameState = CurrentWorld->GetGameState<AOnlineRacingMatchGameState>();
	if (!IsValid(RaceGameState))
	{
		return false;
	}

	if (RaceGameState->GetRacePhase() != EOnlineRacingMatchPhase::Racing)
	{
		return false;
	}

	const AOnlineRacingMatchPlayerState* const RacePlayerState = VehiclePawn->GetPlayerState<AOnlineRacingMatchPlayerState>();
	if (!IsValid(RacePlayerState))
	{
		return false;
	}

	return !RacePlayerState->HasFinishedRace();
}

void AOnlineRacingAIController::SetDrivingEnabled(bool bEnabled)
{
	if (!VehiclePawn.IsValid() || bDrivingEnabled == bEnabled)
	{
		return;
	}

	bDrivingEnabled = bEnabled;
	VehiclePawn->SetRaceInputEnabled(bDrivingEnabled);
}

void AOnlineRacingAIController::FindDrivingLine()
{
	AOnlineRacingDrivingLine* FoundDrivingLine = nullptr;
	uint32 DrivingLineCount = 0;

	for (TActorIterator<AOnlineRacingDrivingLine> It(GetWorld()); It; ++It)
	{
		AOnlineRacingDrivingLine* const Candidate = *It;
		if (!IsValid(Candidate))
		{
			continue;
		}

		++DrivingLineCount;

		if (!FoundDrivingLine)
		{
			FoundDrivingLine = Candidate;
		}
	}

	if (DrivingLineCount != 1 || !IsValid(FoundDrivingLine))
	{
		UE_LOG(
			LogOnlineRacing,
			Error,
			TEXT("[Server][VehicleAI] Expected exactly one DrivingLine, found %u."),
			DrivingLineCount);
		return;
	}

	DrivingLine = FoundDrivingLine;

	const USplineComponent* const SplineComponent = DrivingLine->GetSplineComponent();

	UE_LOG(
		LogOnlineRacing,
		Display,
		TEXT("[Server][VehicleAI] Found DrivingLine %s, length %.0f cm."),
		*GetNameSafe(DrivingLine.Get()),
		SplineComponent->GetSplineLength());
}

bool AOnlineRacingAIController::UpdateStuckDetection(float DeltaSeconds)
{
	if (RecoveryCooldownRemaining > 0.f)
	{
		RecoveryCooldownRemaining = FMath::Max(
			RecoveryCooldownRemaining - DeltaSeconds,
			0.f);

		StuckTime = 0.f;
		return false;
	}

	if (!VehiclePawn.IsValid())
	{
		StuckTime = 0.f;
		return false;
	}

	const UOnlineRacingVehicleTelemetryComponent* const Telemetry = VehiclePawn->GetVehicleTelemetry();
	if (!IsValid(Telemetry))
	{
		StuckTime = 0.f;
		return false;
	}

	const bool bMovingSlowly = Telemetry->GetSpeedKmh() < StuckSpeedThresholdKmh;

	const bool bTryingToMove = Telemetry->GetThrottleInput() >= StuckThrottleThreshold;

	if (!bMovingSlowly || !bTryingToMove)
	{
		StuckTime = 0.f;
		return false;
	}

	StuckTime += DeltaSeconds;

	return StuckTime >= StuckTimeThreshold;
}

void AOnlineRacingAIController::RequestRecovery()
{
	if (!HasAuthority())
	{
		return;
	}

	const UWorld* CurrentWorld = GetWorld();
	if (!IsValid(CurrentWorld))
	{
		return;
	}

	AOnlineRacingMatchGameMode* const RaceGameMode = CurrentWorld->GetAuthGameMode<AOnlineRacingMatchGameMode>();
	if (!IsValid(RaceGameMode))
	{
		UE_LOG(
			LogOnlineRacing,
			Warning,
			TEXT("[Server][VehicleAI] Cannot recover %s because RaceGameMode is unavailable."),
			*GetNameSafe(this));
		return;
	}

	SetDrivingEnabled(false);

	StuckTime = 0.f;

	if (!RaceGameMode->HandleRespawnRequest(*this))
	{
		return;
	}

	RecoveryCooldownRemaining = RecoveryCooldown;

	UE_LOG(
		LogOnlineRacing,
		Display,
		TEXT("[Server][VehicleAI] Requested checkpoint recovery for %s."),
		*GetNameSafe(VehiclePawn.Get()));
}

void AOnlineRacingAIController::UpdateLateralOffset(float DeltaSeconds)
{
	CurrentLateralOffsetCm = FMath::FInterpTo(CurrentLateralOffsetCm, TargetLateralOffsetCm, DeltaSeconds, LaneChangeInterpolationSpeed);

	if (FMath::IsNearlyEqual(CurrentLateralOffsetCm, TargetLateralOffsetCm, LaneChangeCompletionToleranceCm))
	{
		CurrentLateralOffsetCm = TargetLateralOffsetCm;
	}
}

float AOnlineRacingAIController::CalculateLaneOffsetCm(int32 LaneIndex) const
{
	if (DrivingLaneCount <= 0 || LaneIndex < 0 || LaneIndex >= DrivingLaneCount)
	{
		return 0.f;
	}

	const float FirstLaneOffsetCm = -static_cast<float>(DrivingLaneCount - 1) * LaneSpacingCm * 0.5f;

	return FirstLaneOffsetCm + static_cast<float>(LaneIndex) * LaneSpacingCm;
}

bool AOnlineRacingAIController::IsLaneBlocked(int32 LaneIndex, float CurrentSplineDistance) const
{
	if (!VehiclePawn.IsValid() || !DrivingLine.IsValid() || DrivingLaneCount <= 0 || LaneIndex < 0 || LaneIndex >= DrivingLaneCount)
	{
		return true;
	}

	const UWorld* const CurrentWorld = GetWorld();
	if (!IsValid(CurrentWorld))
	{
		return true;
	}

	const USplineComponent* const SplineComponent = DrivingLine->GetSplineComponent();
	if (!IsValid(SplineComponent))
	{
		return true;
	}

	const float LaneOffsetCm = CalculateLaneOffsetCm(LaneIndex);

	const float StartDistance = NormalizeSplineDistance(*SplineComponent, CurrentSplineDistance);

	const float EndDistance = NormalizeSplineDistance(*SplineComponent, CurrentSplineDistance + VehicleDetectionDistanceCm);

	const FVector StartCenter = SplineComponent->GetLocationAtDistanceAlongSpline(StartDistance, ESplineCoordinateSpace::World);

	const FVector StartRight = SplineComponent->GetRightVectorAtDistanceAlongSpline(StartDistance, ESplineCoordinateSpace::World);

	const FVector StartUp = SplineComponent->GetUpVectorAtDistanceAlongSpline(StartDistance, ESplineCoordinateSpace::World);

	const FVector EndCenter = SplineComponent->GetLocationAtDistanceAlongSpline(EndDistance, ESplineCoordinateSpace::World);

	const FVector EndRight = SplineComponent->GetRightVectorAtDistanceAlongSpline(EndDistance, ESplineCoordinateSpace::World);

	const FVector EndUp = SplineComponent->GetUpVectorAtDistanceAlongSpline(EndDistance, ESplineCoordinateSpace::World);

	const FVector TraceStart = StartCenter + StartRight * LaneOffsetCm + StartUp * VehicleDetectionHeightCm;

	const FVector TraceEnd = EndCenter + EndRight * LaneOffsetCm + EndUp * VehicleDetectionHeightCm;

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Vehicle);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(OnlineRacingAILaneDetection), false, VehiclePawn.Get());

	TArray<FHitResult> Hits;

	CurrentWorld->SweepMultiByObjectType(
		Hits,
		TraceStart,
		TraceEnd,
		FQuat::Identity,
		ObjectQueryParams,
		FCollisionShape::MakeSphere(VehicleDetectionRadiusCm),
		QueryParams);

	bool bLaneBlocked = false;

	for (const FHitResult& Hit : Hits)
	{
		const AOnlineRacingPawn* const OtherVehicle = Cast<AOnlineRacingPawn>(Hit.GetActor());

		if (IsValid(OtherVehicle) && OtherVehicle != VehiclePawn.Get())
		{
			bLaneBlocked = true;
			break;
		}
	}

	if (bDrawDrivingDebug)
	{
		FColor DebugColor = FColor::Green;

		if (bLaneBlocked)
		{
			DebugColor = FColor::Red;
		}

		DrawDebugLine(
			CurrentWorld,
			TraceStart,
			TraceEnd,
			DebugColor,
			false,
			0.f,
			0,
			4.f
		);

		DrawDebugSphere(
			CurrentWorld,
			TraceEnd,
			VehicleDetectionRadiusCm,
			12,
			DebugColor,
			false,
			0.f);
	}

	return bLaneBlocked;
}

bool AOnlineRacingAIController::TryChangeToFreeAdjacentLane(float CurrentSplineDistance)
{
	if (TargetLaneIndex == INDEX_NONE || DrivingLaneCount <= 0)
	{
		return false;
	}

	const int32 LeftLaneIndex = TargetLaneIndex - 1;

	if (LeftLaneIndex >= 0 && !IsLaneBlocked(LeftLaneIndex, CurrentSplineDistance))
	{
		TargetLaneIndex = LeftLaneIndex;
		TargetLateralOffsetCm = CalculateLaneOffsetCm(TargetLaneIndex);

		return true;
	}

	const int32 RightLaneIndex = TargetLaneIndex + 1;

	if (RightLaneIndex < DrivingLaneCount && !IsLaneBlocked(RightLaneIndex, CurrentSplineDistance))
	{
		TargetLaneIndex = RightLaneIndex;
		TargetLateralOffsetCm = CalculateLaneOffsetCm(TargetLaneIndex);

		return true;
	}

	return false;
}

bool AOnlineRacingAIController::IsLaneChangeInProgress() const
{
	return !FMath::IsNearlyEqual(CurrentLateralOffsetCm, TargetLateralOffsetCm, LaneChangeCompletionToleranceCm);
}

float AOnlineRacingAIController::NormalizeSplineDistance(const USplineComponent& SplineComponent, const float Distance)
{
	const float SplineLength = SplineComponent.GetSplineLength();
	if (FMath::IsNearlyZero(SplineLength))
	{
		return 0.f;
	}

	if (SplineComponent.IsClosedLoop())
	{
		float NormalizedDistance = FMath::Fmod(Distance, SplineLength);

		if (NormalizedDistance < 0.f)
		{
			NormalizedDistance += SplineLength;
		}

		return NormalizedDistance;
	}

	return FMath::Clamp(Distance, 0.f, SplineLength);
}

bool AOnlineRacingAIController::TryGetDrivingTarget(FVector& OutTargetLocation, float& OutCurrentSplineDistance) const
{

	OutTargetLocation = FVector::ZeroVector;
	OutCurrentSplineDistance = 0.f;

	if (!VehiclePawn.IsValid() || !DrivingLine.IsValid())
	{
		return false;
	}

	const USplineComponent* const SplineComponent = DrivingLine->GetSplineComponent();
	if (!IsValid(SplineComponent))
	{
		return false;
	}

	const float SplineLength = SplineComponent->GetSplineLength();
	if (FMath::IsNearlyZero(SplineLength))
	{
		return false;
	}

	const FVector VehicleLocation = VehiclePawn->GetActorLocation();

	const float ClosestInputKey = SplineComponent->FindInputKeyClosestToWorldLocation(VehicleLocation);

	const float CurrentDistance = SplineComponent->GetDistanceAlongSplineAtSplineInputKey(ClosestInputKey);
	OutCurrentSplineDistance = CurrentDistance;

	float TargetDistance = CurrentDistance + LookAheadDistance;

	TargetDistance = NormalizeSplineDistance(*SplineComponent, TargetDistance);

	const FVector TargetCenter =  SplineComponent->GetLocationAtDistanceAlongSpline(TargetDistance, ESplineCoordinateSpace::World);

	const FVector TargetRight = SplineComponent->GetRightVectorAtDistanceAlongSpline(TargetDistance, ESplineCoordinateSpace::World);

	OutTargetLocation = TargetCenter + TargetRight * CurrentLateralOffsetCm;

	return true;
}

float AOnlineRacingAIController::CalculateSteeringInput(const FVector& TargetLocation) const
{
	if (!VehiclePawn.IsValid())
	{
		return 0.f;
	}

	const FVector LocalTarget = VehiclePawn->GetActorTransform().InverseTransformPositionNoScale(TargetLocation);

	const float TargetAngleRadians = static_cast<float>(FMath::Atan2(LocalTarget.Y, LocalTarget.X));

	const float NormalizedAngle = TargetAngleRadians / UE_HALF_PI;

	return FMath::Clamp(NormalizedAngle * SteeringGain, -1.f, 1.f);
}

float AOnlineRacingAIController::CalculateTargetSpeedKmh(float TurnAmount) const
{
	const float NormalizedTurnAmount = FMath::Clamp(TurnAmount, 0.f, 1.f);

	return FMath::Lerp(StraightTargetSpeedKmh, CornerTargetSpeedKmh, NormalizedTurnAmount);
}

float AOnlineRacingAIController::CalculateDirectionChangeAmount(const FVector& FromDirection, const FVector& ToDirection) const
{
	const FVector NormalizedFrom = FromDirection.GetSafeNormal2D();

	const FVector NormalizedTo = ToDirection.GetSafeNormal2D();

	if (NormalizedFrom.IsNearlyZero() || NormalizedTo.IsNearlyZero())
	{
		return 0.f;
	}

	const float DirectionDot = FMath::Clamp(FVector::DotProduct(NormalizedFrom, NormalizedTo), -1.f, 1.f);

	const float TurnAngleRadians = FMath::Acos(DirectionDot);

	const float TurnAngleDegrees = FMath::RadiansToDegrees(TurnAngleRadians);

	return FMath::Clamp(TurnAngleDegrees / FullTurnAngleDegrees, 0.f, 1.f);
}

float AOnlineRacingAIController::CalculateUpcomingTurnAmount(const float CurrentSplineDistance) const
{
	if (!DrivingLine.IsValid())
	{
		return 0.f;
	}

	const USplineComponent* const SplineComponent = DrivingLine->GetSplineComponent();
	if (!IsValid(SplineComponent))
	{
		return 0.f;
	}

	const float NearDistance = NormalizeSplineDistance(*SplineComponent, CurrentSplineDistance + NearTurnSampleDistance);

	const float FarDistance = NormalizeSplineDistance(*SplineComponent, CurrentSplineDistance + FarTurnSampleDistance);

	const FVector CurrentDirection = SplineComponent->GetDirectionAtDistanceAlongSpline(
		NormalizeSplineDistance(*SplineComponent, CurrentSplineDistance),
		ESplineCoordinateSpace::World);

	const FVector NearDirection = SplineComponent->GetDirectionAtDistanceAlongSpline(NearDistance, ESplineCoordinateSpace::World);

	const FVector FarDirection = SplineComponent->GetDirectionAtDistanceAlongSpline(FarDistance, ESplineCoordinateSpace::World);

	const float NearTurnAmount = CalculateDirectionChangeAmount(CurrentDirection, NearDirection);

	const float FarTurnAmount = CalculateDirectionChangeAmount(NearDirection, FarDirection);

	const float WeightedFarTurnAmount = FarTurnAmount * FarTurnInfluence;

	return FMath::Max(NearTurnAmount, WeightedFarTurnAmount);
}

bool AOnlineRacingAIController::ApplyDrivingInput(float SteeringInput, float TargetSpeedKmh)
{
	if (!VehiclePawn.IsValid())
	{
		return false;
	}

	const UOnlineRacingVehicleTelemetryComponent* const Telemetry = VehiclePawn->GetVehicleTelemetry();
	if (!IsValid(Telemetry))
	{
		return false;
	}

	const float CurrentSpeedKmh = Telemetry->GetSpeedKmh();

	VehiclePawn->DoSteering(SteeringInput);

	if (CurrentSpeedKmh > TargetSpeedKmh + BrakeDeadZoneKmh)
	{
		const float BrakeInput = FMath::Clamp((CurrentSpeedKmh - TargetSpeedKmh) / SpeedControlRangeKmh, 0.f, 1.f);

		VehiclePawn->DoBrake(BrakeInput);
		return true;
	}

	const float ThrottleInput = FMath::Clamp((TargetSpeedKmh - CurrentSpeedKmh) / SpeedControlRangeKmh, 0.f, 1.f);

	VehiclePawn->DoThrottle(ThrottleInput);

	return true;
}

void AOnlineRacingAIController::DrawDrivingDebug(
	const FVector& TargetLocation,
	const float SteeringInput,
	const float UpcomingTurnAmount,
	float TargetSpeedKmh,
	bool bIsLaneBlocked) const
{
	if (!VehiclePawn.IsValid())
	{
		return;
	}

	const UWorld* const World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	const UOnlineRacingVehicleTelemetryComponent* const Telemetry = VehiclePawn->GetVehicleTelemetry();

	float BrakeInput = 0.f;

	if (IsValid(Telemetry))
	{
		BrakeInput = Telemetry->GetBrakeInput();
	}

	const FVector DebugOffset(0.f, 0.f, 100.f);

	const FVector DebugVehicleLocation =
		VehiclePawn->GetActorLocation() + DebugOffset;

	const FVector DebugTargetLocation =
		TargetLocation + DebugOffset;

	DrawDebugLine(
		World,
		DebugVehicleLocation,
		DebugTargetLocation,
		FColor::Cyan,
		false,
		0.f,
		0,
		5.f);

	DrawDebugSphere(
		World,
		DebugTargetLocation,
		35.f,
		12,
		FColor::Yellow,
		false,
		0.f);

	DrawDebugString(
		World,
		DebugTargetLocation + FVector(0.f, 0.f, 60.f),
		FString::Printf(
	TEXT(
		"Steering: %.2f\n"
		"Upcoming turn: %.2f\n"
		"Target speed: %.1f km/h\n"
		"Brake: %.2f\n"
		"Lane blocked: %s"),
		SteeringInput,
		UpcomingTurnAmount,
		TargetSpeedKmh,
		BrakeInput,
		bIsLaneBlocked ? TEXT("yes") : TEXT("no")),
		nullptr,
		FColor::White,
		0.f,
		true);
}

void AOnlineRacingAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!HasAuthority())
	{
		return;
	}

	if (!CanDrive())
	{
		SetDrivingEnabled(false);
		StuckTime = 0.f;
		RecoveryCooldownRemaining = 0.f;
		return;
	}

	UpdateLateralOffset(DeltaSeconds);

	FVector TargetLocation = FVector::ZeroVector;
	float CurrentSplineDistance = 0.f;

	if (!TryGetDrivingTarget(TargetLocation, CurrentSplineDistance))
	{
		SetDrivingEnabled(false);
		return;
	}

	bool bIsLaneBlocked = IsLaneBlocked(TargetLaneIndex, CurrentSplineDistance);

	if (bIsLaneBlocked && !IsLaneChangeInProgress() && TryChangeToFreeAdjacentLane(CurrentSplineDistance))
	{
		bIsLaneBlocked = false;
	}

	SetDrivingEnabled(true);

	const float SteeringInput = CalculateSteeringInput(TargetLocation);

	const float UpcomingTurnAmount = CalculateUpcomingTurnAmount(CurrentSplineDistance);

	const float ImmediateTurnAmount = FMath::Clamp(FMath::Abs(SteeringInput), 0.f, 1.f);

	const float SpeedTurnAmount = FMath::Max(ImmediateTurnAmount, UpcomingTurnAmount);

	float TargetSpeedKmh = CalculateTargetSpeedKmh(SpeedTurnAmount);

	if (bIsLaneBlocked)
	{
		TargetSpeedKmh = FMath::Min(TargetSpeedKmh, BlockedLaneTargetSpeedKmh);
	}

	if (!ApplyDrivingInput(SteeringInput, TargetSpeedKmh))
	{
		SetDrivingEnabled(false);
		return;
	}

	if (UpdateStuckDetection(DeltaSeconds))
	{
		RequestRecovery();
		return;
	}

	if (bDrawDrivingDebug)
	{
		DrawDrivingDebug(TargetLocation, SteeringInput, UpcomingTurnAmount, TargetSpeedKmh, bIsLaneBlocked);
	}
}

void AOnlineRacingAIController::ConfigureDrivingLane(int32 LaneIndex, int32 LaneCount)
{
	if (LaneCount <= 0 || LaneIndex < 0 || LaneIndex >= LaneCount)
	{
		return;
	}

	BaseLaneIndex = LaneIndex;
	TargetLaneIndex = LaneIndex;
	DrivingLaneCount = LaneCount;

	TargetLateralOffsetCm = CalculateLaneOffsetCm(TargetLaneIndex);

	CurrentLateralOffsetCm = TargetLateralOffsetCm;
}