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
	
	OutTargetLocation = SplineComponent->GetLocationAtDistanceAlongSpline(TargetDistance, ESplineCoordinateSpace::World);
		
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
	
	const FVector NearDirection = SplineComponent->GetDirectionAtDistanceAlongSpline(NearDistance, ESplineCoordinateSpace::World).GetSafeNormal2D();
	
	const FVector FarDirection = SplineComponent->GetDirectionAtDistanceAlongSpline(FarDistance, ESplineCoordinateSpace::World).GetSafeNormal2D();
	
	if (NearDirection.IsNearlyZero() || FarDirection.IsNearlyZero())
	{
		return 0.f;
	}
	
	const float DirectionDot = FMath::Clamp(FVector::DotProduct(NearDirection, FarDirection), -1.f, 1.f);
	
	const float TurnAngleRadians = FMath::Acos(DirectionDot);
	
	const float TurnAngleDegrees = FMath::RadiansToDegrees(TurnAngleRadians);
	
	return FMath::Clamp(TurnAngleDegrees / FullTurnAngleDegrees, 0.f, 1.f);
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
	const float UpcomingTurnAmount) const
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
			TEXT("Steering: %.2f\nUpcoming turn: %.2f"),
			SteeringInput,
			UpcomingTurnAmount),
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
	
	FVector TargetLocation = FVector::ZeroVector;
	float CurrentSplineDistance = 0.f;
	
	if (!TryGetDrivingTarget(TargetLocation, CurrentSplineDistance))
	{
		SetDrivingEnabled(false);
		return;
	}
	
	SetDrivingEnabled(true);
	
	const float SteeringInput = CalculateSteeringInput(TargetLocation);
	
	const float UpcomingTurnAmount = CalculateUpcomingTurnAmount(CurrentSplineDistance);
	
	const float ImmediateTurnAmount = FMath::Clamp(FMath::Abs(SteeringInput), 0.f, 1.f);
	
	const float SpeedTurnAmount = FMath::Max(ImmediateTurnAmount, UpcomingTurnAmount);
	
	const float TargetSpeedKmh = CalculateTargetSpeedKmh(SpeedTurnAmount);
	
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
		DrawDrivingDebug(TargetLocation, SteeringInput, UpcomingTurnAmount);
	}
}
