// Copyright Epic Games, Inc. All Rights Reserved.

#include "OnlineRacingPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/GameModeBase.h"
#include "InputMappingContext.h"
#include "Widgets/Input/SVirtualJoystick.h"

#include "OnlineRacing.h"
#include "OnlineRacingPawn.h"
#include "OnlineRacingUI.h"

void AOnlineRacingPlayerController::BeginPlay()
{
	Super::BeginPlay();

	bAttachToPawn = true;
	CacheVehiclePawn();
}

void AOnlineRacingPlayerController::SetupInputComponent()
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

	if (ShouldUseTouchControls() && !IsValid(MobileControlsWidget))
	{
		MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);
		if (IsValid(MobileControlsWidget))
		{
			MobileControlsWidget->AddToPlayerScreen(0);
		}
		else
		{
			UE_LOG(LogOnlineRacing, Error, TEXT("[Client][PlayerController] Failed to create mobile controls widget for %s."), *GetNameSafe(this));
		}
	}

	if (!IsValid(VehicleUI))
	{
		VehicleUI = CreateWidget<UOnlineRacingUI>(this, VehicleUIClass);
		if (IsValid(VehicleUI))
		{
			VehicleUI->AddToPlayerScreen(0);
		}
		else
		{
			UE_LOG(LogOnlineRacing, Error, TEXT("[Client][PlayerController] Failed to create vehicle UI for %s."), *GetNameSafe(this));
		}
	}
}

void AOnlineRacingPlayerController::Tick(const float DeltaSeconds)
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

void AOnlineRacingPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	CacheVehiclePawn();
}

void AOnlineRacingPlayerController::OnRep_Pawn()
{
	Super::OnRep_Pawn();
	CacheVehiclePawn();
}

void AOnlineRacingPlayerController::CacheVehiclePawn()
{
	AOnlineRacingPawn* const NewVehiclePawn = Cast<AOnlineRacingPawn>(GetPawn());
	if (VehiclePawn == NewVehiclePawn)
	{
		return;
	}

	if (IsValid(VehiclePawn))
	{
		VehiclePawn->OnDestroyed.RemoveDynamic(this, &AOnlineRacingPlayerController::OnPawnDestroyed);
	}

	VehiclePawn = NewVehiclePawn;
	if (IsValid(VehiclePawn))
	{
		VehiclePawn->OnDestroyed.AddUniqueDynamic(this, &AOnlineRacingPlayerController::OnPawnDestroyed);
		return;
	}

	if (IsValid(GetPawn()))
	{
		UE_LOG(LogOnlineRacing, Warning, TEXT("[Role=%s][PlayerController] Pawn %s is not an OnlineRacing vehicle."), *UEnum::GetValueAsString(GetLocalRole()), *GetNameSafe(GetPawn()));
	}
}

void AOnlineRacingPlayerController::OnPawnDestroyed(AActor* DestroyedPawn)
{
	VehiclePawn = nullptr;

	if (!HasAuthority())
	{
		return;
	}

	AGameModeBase* const GameMode = GetWorld()->GetAuthGameMode();
	if (!IsValid(GameMode))
	{
		UE_LOG(LogOnlineRacing, Warning, TEXT("[Server][PlayerController] Cannot respawn %s because no authoritative GameMode exists."), *GetNameSafe(this));
		return;
	}

	GameMode->RestartPlayer(this);
}

bool AOnlineRacingPlayerController::ShouldUseTouchControls() const
{
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}
