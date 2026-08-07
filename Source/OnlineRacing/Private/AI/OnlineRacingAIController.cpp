// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/OnlineRacingAIController.h"

#include "EngineUtils.h"
#include "OnlineRacing.h"
#include "OnlineRacingPawn.h"
#include "AI/OnlineRacingRacingLine.h"
#include "Components/SplineComponent.h"
#include "GameFramework/PlayerState.h"


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
	
	FindRacingLine();
}

void AOnlineRacingAIController::FindRacingLine()
{
	AOnlineRacingRacingLine* FoundRacingLine = nullptr;
	uint32 RacingLineCount = 0;
	
	for (TActorIterator<AOnlineRacingRacingLine> It(GetWorld()); It; ++It)
	{
		AOnlineRacingRacingLine* const Candidate = *It;
		if (!IsValid(Candidate))
		{
			continue;
		}
		
		++RacingLineCount;
		
		if (!FoundRacingLine)
		{
			FoundRacingLine = Candidate;
		}
	}
	
	if (RacingLineCount != 1 || !IsValid(FoundRacingLine))
	{
		UE_LOG(
			LogOnlineRacing,
			Error,
			TEXT("[Server][VehicleAI] Expected exactly one RacingLine, found %d."),
			RacingLineCount);
		return;
	}

	RacingLine = FoundRacingLine;

	const USplineComponent* const SplineComponent = RacingLine->GetSplineComponent();

	UE_LOG(
		LogOnlineRacing,
		Display,
		TEXT("[Server][VehicleAI] Found RacingLine %s, length %.0f cm."),
		*GetNameSafe(RacingLine.Get()),
		SplineComponent->GetSplineLength());
}

void AOnlineRacingAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	
	if (!HasAuthority() || !bDrawDrivingDebug)
	{
		return;
	}
	
	DrawDrivingTarget();
}

void AOnlineRacingAIController::DrawDrivingTarget() const
{
	const UWorld* CurrentWorld = GetWorld();
	if (!IsValid(CurrentWorld))
	{
		return;
	}
	
	if (!VehiclePawn.IsValid() || !RacingLine.IsValid())
	{
		return;
	}
	
	const USplineComponent* const SplineComponent = RacingLine->GetSplineComponent();
	if (!IsValid(SplineComponent))
	{
		return;
	}
	
	const float SplineLength = SplineComponent->GetSplineLength();
	if (FMath::IsNearlyZero(SplineLength))
	{
		return;
	}
	
	const FVector VehicleLocation = VehiclePawn->GetActorLocation();
	
	const float ClosestInputKey = SplineComponent->FindInputKeyClosestToWorldLocation(VehicleLocation);
	
	const float CurrentDistance = SplineComponent->GetDistanceAlongSplineAtSplineInputKey(ClosestInputKey);
	
	float TargetDistance = CurrentDistance + LookAheadDistance;
	
	if (SplineComponent->IsClosedLoop())
	{
		TargetDistance = FMath::Fmod(TargetDistance, SplineLength);
	}
	else
	{
		TargetDistance = FMath::Clamp(TargetDistance, 0.f, SplineLength);
	}
	
	const FVector TargetLocation = SplineComponent->GetLocationAtDistanceAlongSpline(TargetDistance, ESplineCoordinateSpace::World);
	
	const FVector DebugOffset = FVector::UpVector * 100.0;
	
	DrawDebugLine(CurrentWorld, VehicleLocation + DebugOffset, TargetLocation + DebugOffset, FColor::Cyan, false, 0.f, 0, 5.f);
	
	DrawDebugSphere(CurrentWorld, TargetLocation + DebugOffset, 35.f, 12, FColor::Yellow, false, 0.f);
}
