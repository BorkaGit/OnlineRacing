// Copyright Epic Games, Inc. All Rights Reserved.

#include "TimeTrialPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/GameModeBase.h"
#include "InputMappingContext.h"
#include "Widgets/Input/SVirtualJoystick.h"

#include "OnlineRacing.h"
#include "Vehicle/OnlineRacingPawn.h"
#include "UI/OnlineRacingUI.h"
#include "TimeTrialGameMode.h"
#include "TimeTrialTrackGate.h"
#include "TimeTrialUI.h"

void ATimeTrialPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (!IsLocalPlayerController())
	{
		return;
	}

	UEnhancedInputLocalPlayerSubsystem* const InputSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
	if (IsValid(InputSubsystem))
	{
		for (UInputMappingContext* const MappingContext : DefaultMappingContexts)
		{
			if (IsValid(MappingContext))
			{
				InputSubsystem->AddMappingContext(MappingContext, 0);
			}
		}

		if (!ShouldUseTouchControls())
		{
			for (UInputMappingContext* const MappingContext : MobileExcludedMappingContexts)
			{
				if (IsValid(MappingContext))
				{
					InputSubsystem->AddMappingContext(MappingContext, 0);
				}
			}
		}

		if (bUseSteeringWheelControls && IsValid(SteeringWheelInputMappingContext.Get()))
		{
			InputSubsystem->AddMappingContext(SteeringWheelInputMappingContext, 0);
		}
	}

	if (ShouldUseTouchControls())
	{
		MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);
		if (IsValid(MobileControlsWidget))
		{
			MobileControlsWidget->AddToPlayerScreen(0);
		}
		else
		{
			UE_LOG(LogOnlineRacing, Error, TEXT("[Client][TimeTrial] Failed to create mobile controls widget."));
		}
	}

	UIWidget = CreateWidget<UTimeTrialUI>(this, UIWidgetClass);
	if (IsValid(UIWidget))
	{
		UIWidget->AddToPlayerScreen(0);
		UIWidget->OnRaceStart.AddDynamic(this, &ATimeTrialPlayerController::StartRace);
	}
	else
	{
		UE_LOG(LogOnlineRacing, Error, TEXT("[Client][TimeTrial] Failed to create time-trial UI."));
	}

	VehicleUI = CreateWidget<UOnlineRacingUI>(this, VehicleUIClass);
	if (IsValid(VehicleUI))
	{
		VehicleUI->AddToPlayerScreen(0);
	}
	else
	{
		UE_LOG(LogOnlineRacing, Error, TEXT("[Client][TimeTrial] Failed to create vehicle UI."));
	}
}

void ATimeTrialPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	CacheVehiclePawn();
}

void ATimeTrialPlayerController::OnRep_Pawn()
{
	Super::OnRep_Pawn();
	CacheVehiclePawn();
}

void ATimeTrialPlayerController::CacheVehiclePawn()
{
	AOnlineRacingPawn* const NewVehiclePawn = Cast<AOnlineRacingPawn>(GetPawn());
	if (VehiclePawn == NewVehiclePawn)
	{
		return;
	}

	if (IsValid(VehiclePawn))
	{
		VehiclePawn->OnDestroyed.RemoveDynamic(this, &ATimeTrialPlayerController::OnPawnDestroyed);
	}

	VehiclePawn = NewVehiclePawn;
	if (!IsValid(VehiclePawn))
	{
		return;
	}

	VehiclePawn->OnDestroyed.AddUniqueDynamic(this, &ATimeTrialPlayerController::OnPawnDestroyed);
	if (!bRaceStarted)
	{
		VehiclePawn->DisableInput(this);
	}
}

void ATimeTrialPlayerController::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!IsValid(VehiclePawn) || !IsValid(VehicleUI))
	{
		return;
	}

	const UChaosWheeledVehicleMovementComponent* const MovementComponent = VehiclePawn->GetChaosVehicleMovement();
	if (!IsValid(MovementComponent))
	{
		return;
	}

	VehicleUI->UpdateSpeed(MovementComponent->GetForwardSpeed());
	VehicleUI->UpdateGear(MovementComponent->GetCurrentGear());
}

void ATimeTrialPlayerController::StartRace()
{
	ATimeTrialGameMode* const GameMode = Cast<ATimeTrialGameMode>(GetWorld()->GetAuthGameMode());
	if (IsValid(GameMode) && IsValid(GameMode->GetFinishLine()))
	{
		SetTargetGate(GameMode->GetFinishLine()->GetNextMarker());
	}

	bRaceStarted = true;
	CurrentLap = 0;
	IncrementLapCount();

	if (IsValid(GetPawn()))
	{
		GetPawn()->EnableInput(this);
	}
}

void ATimeTrialPlayerController::IncrementLapCount()
{
	++CurrentLap;

	if (IsValid(UIWidget))
	{
		UIWidget->UpdateLapCount(CurrentLap, GetWorld()->GetTimeSeconds());
	}
}

ATimeTrialTrackGate* ATimeTrialPlayerController::GetTargetGate() const
{
	return TargetGate.Get();
}

void ATimeTrialPlayerController::SetTargetGate(ATimeTrialTrackGate* Gate)
{
	TargetGate = Gate;
}

void ATimeTrialPlayerController::OnPawnDestroyed(AActor* DestroyedPawn)
{
	VehiclePawn = nullptr;

	if (!HasAuthority())
	{
		return;
	}

	AGameModeBase* const GameMode = GetWorld()->GetAuthGameMode();
	if (!IsValid(GameMode))
	{
		UE_LOG(LogOnlineRacing, Warning, TEXT("[Server][TimeTrial] Cannot respawn %s because no authoritative GameMode exists."), *GetNameSafe(this));
		return;
	}

	GameMode->RestartPlayer(this);
}

bool ATimeTrialPlayerController::ShouldUseTouchControls() const
{
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}
