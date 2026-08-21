// Copyright Epic Games, Inc. All Rights Reserved.

#include "Framework/PlayerControllers/OnlineRacingMatchPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/GameModeBase.h"
#include "InputMappingContext.h"
#include "TimerManager.h"
#include "Widgets/Input/SVirtualJoystick.h"

#include "OnlineRacing.h"
#include "Vehicle/OnlineRacingPawn.h"
#include "UI/OnlineRacingUI.h"
#include "Framework/GameModes/OnlineRacingMatchGameMode.h"
#include "Framework/GameStates/OnlineRacingMatchGameState.h"
#include "Framework/PlayerStates/OnlineRacingPlayerState.h"
#include "UI/OnlineRacingDebugWidget.h"
#include "UI/OnlineRacingCountdownWidget.h"
#include "UI/OnlineRacingResultsWidget.h"

void AOnlineRacingMatchPlayerController::BeginPlay()
{
	Super::BeginPlay();

	bAttachToPawn = true;
	CacheVehiclePawn();
	BindRaceGameState();
	BindRacePlayerState();
}

void AOnlineRacingMatchPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(RaceStartedDisplayTimer);
	UnbindRaceGameState();
	UnbindRacePlayerState();
	Super::EndPlay(EndPlayReason);
}

void AOnlineRacingMatchPlayerController::SetupInputComponent()
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

	if (IsValid(DebugWidgetClass.Get()) && !IsValid(DebugWidget))
	{
		DebugWidget = CreateWidget<UOnlineRacingDebugWidget>(this, DebugWidgetClass);
		if (IsValid(DebugWidget))
		{
			DebugWidget->AddToPlayerScreen(10);
		}
		else
		{
			UE_LOG(LogOnlineRacing, Error, TEXT("[Client][PlayerController] Failed to create debug widget for %s."), *GetNameSafe(this));
		}
	}

	if (IsValid(RaceCountdownWidgetClass.Get()) && !IsValid(RaceCountdownWidget))
	{
		RaceCountdownWidget = CreateWidget<UOnlineRacingCountdownWidget>(this, RaceCountdownWidgetClass);
		if (IsValid(RaceCountdownWidget))
		{
			RaceCountdownWidget->AddToPlayerScreen(20);
			bRaceStartPresented = false;
		}
		else
		{
			UE_LOG(LogOnlineRacing, Error, TEXT("[Client][PlayerController] Failed to create race countdown widget for %s."), *GetNameSafe(this));
		}
	}

	if (IsValid(RaceResultsWidgetClass.Get()) && !IsValid(RaceResultsWidget))
	{
		RaceResultsWidget = CreateWidget<UOnlineRacingResultsWidget>(this, RaceResultsWidgetClass);
		if (IsValid(RaceResultsWidget))
		{
			RaceResultsWidget->AddToPlayerScreen(30);
		}
		else
		{
			UE_LOG(LogOnlineRacing, Error, TEXT("[Client][PlayerController] Failed to create race results widget for %s."), *GetNameSafe(this));
		}
	}

	BindRaceGameState();
	if (RaceGameState.IsValid())
	{
		HandleRacePhaseChanged(RaceGameState->GetRacePhase());
	}
}

void AOnlineRacingMatchPlayerController::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	BindRaceGameState();
	UpdateRaceCountdown();

	if (!IsValid(VehiclePawn))
	{
		return;
	}

	if (IsValid(VehicleUI))
	{
		const UChaosWheeledVehicleMovementComponent* const MovementComponent = VehiclePawn->GetChaosVehicleMovement();
		if (IsValid(MovementComponent))
		{
			VehicleUI->UpdateSpeed(MovementComponent->GetForwardSpeed());
			VehicleUI->UpdateGear(MovementComponent->GetCurrentGear());
		}
	}

	if (IsValid(DebugWidget))
	{
		DebugWidget->UpdateDebugData(*VehiclePawn);
	}
}

void AOnlineRacingMatchPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	BindRacePlayerState();
	CacheVehiclePawn();
}

void AOnlineRacingMatchPlayerController::OnRep_Pawn()
{
	Super::OnRep_Pawn();
	CacheVehiclePawn();
}

void AOnlineRacingMatchPlayerController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	BindRacePlayerState();
}

void AOnlineRacingMatchPlayerController::RequestVehicleRespawn()
{
	if (HasAuthority())
	{
		HandleVehicleRespawnRequest();
		return;
	}

	Server_RequestVehicleRespawn();
}

void AOnlineRacingMatchPlayerController::Server_RequestVehicleRespawn_Implementation()
{
	HandleVehicleRespawnRequest();
}

void AOnlineRacingMatchPlayerController::HandleVehicleRespawnRequest()
{
	if (!HasAuthority())
	{
		UE_LOG(LogOnlineRacing, Warning, TEXT("[Client][PlayerController] Rejected non-authoritative respawn handling for %s."), *GetNameSafe(this));
		return;
	}

	if (!IsValid(VehiclePawn))
	{
		CacheVehiclePawn();
	}

	if (!IsValid(VehiclePawn))
	{
		UE_LOG(LogOnlineRacing, Warning, TEXT("[Server][PlayerController] Cannot respawn %s because it has no OnlineRacing vehicle."), *GetNameSafe(this));
		return;
	}

	AOnlineRacingMatchGameMode* const RaceGameMode = GetWorld()->GetAuthGameMode<AOnlineRacingMatchGameMode>();
	if (IsValid(RaceGameMode))
	{
		RaceGameMode->HandleRespawnRequest(*this);
		return;
	}

	VehiclePawn->ResetVehicleAtCurrentLocation();
}

void AOnlineRacingMatchPlayerController::BindRaceGameState()
{
	AOnlineRacingMatchGameState* const NewRaceGameState = GetWorld()->GetGameState<AOnlineRacingMatchGameState>();
	if (RaceGameState == NewRaceGameState)
	{
		return;
	}

	UnbindRaceGameState();
	RaceGameState = NewRaceGameState;
	if (!RaceGameState.IsValid())
	{
		return;
	}

	RacePhaseChangedHandle = RaceGameState->OnRacePhaseChanged().AddUObject(this, &AOnlineRacingMatchPlayerController::HandleRacePhaseChanged);
	RaceResultsChangedHandle = RaceGameState->OnRaceResultsChanged().AddUObject(this, &AOnlineRacingMatchPlayerController::HandleRaceResultsChanged);
	HandleRacePhaseChanged(RaceGameState->GetRacePhase());
}

void AOnlineRacingMatchPlayerController::UnbindRaceGameState()
{
	if (RaceGameState.IsValid() && RacePhaseChangedHandle.IsValid())
	{
		RaceGameState->OnRacePhaseChanged().Remove(RacePhaseChangedHandle);
	}
	if (RaceGameState.IsValid() && RaceResultsChangedHandle.IsValid())
	{
		RaceGameState->OnRaceResultsChanged().Remove(RaceResultsChangedHandle);
	}

	RacePhaseChangedHandle.Reset();
	RaceResultsChangedHandle.Reset();
	RaceGameState.Reset();
}

void AOnlineRacingMatchPlayerController::BindRacePlayerState()
{
	AOnlineRacingPlayerState* const NewRacePlayerState = GetPlayerState<AOnlineRacingPlayerState>();
	if (RacePlayerState == NewRacePlayerState)
	{
		return;
	}

	UnbindRacePlayerState();
	RacePlayerState = NewRacePlayerState;
	if (!RacePlayerState.IsValid())
	{
		return;
	}

	RaceFinishedChangedHandle = RacePlayerState->OnRaceFinishedChanged().AddUObject(
		this,
		&AOnlineRacingMatchPlayerController::HandlePlayerFinishedChanged);
	HandlePlayerFinishedChanged(RacePlayerState->HasFinishedRace());
}

void AOnlineRacingMatchPlayerController::UnbindRacePlayerState()
{
	if (RacePlayerState.IsValid() && RaceFinishedChangedHandle.IsValid())
	{
		RacePlayerState->OnRaceFinishedChanged().Remove(RaceFinishedChangedHandle);
	}

	RaceFinishedChangedHandle.Reset();
	RacePlayerState.Reset();
}

void AOnlineRacingMatchPlayerController::HandleRacePhaseChanged(const EOnlineRacingMatchPhase NewRacePhase)
{
	switch (NewRacePhase)
	{
	case EOnlineRacingMatchPhase::Waiting:
		bRaceStartPresented = false;
		SetVehicleRaceInputEnabled(false);
		SetDebugWidgetVisible(true);
		HideCountdownWidget();
		HideRaceResults();
		break;
	case EOnlineRacingMatchPhase::Countdown:
		bRaceStartPresented = false;
		SetVehicleRaceInputEnabled(false);
		SetDebugWidgetVisible(true);
		HideRaceResults();
		UpdateRaceCountdown();
		break;
	case EOnlineRacingMatchPhase::Racing:
		ApplyRaceInputStateToVehicle();
		SetDebugWidgetVisible(true);
		HideRaceResults();
		PresentRaceStart();
		break;
	case EOnlineRacingMatchPhase::Finished:
		SetVehicleRaceInputEnabled(false);
		SetDebugWidgetVisible(false);
		HideCountdownWidget();
		ShowRaceResults();
		break;
	default:
		break;
	}
}

void AOnlineRacingMatchPlayerController::HandleRaceResultsChanged(const TArray<FOnlineRacingMatchResult>&)
{
	if (!RaceGameState.IsValid() || RaceGameState->GetRacePhase() != EOnlineRacingMatchPhase::Finished)
	{
		return;
	}

	ShowRaceResults();
}

void AOnlineRacingMatchPlayerController::HandlePlayerFinishedChanged(const bool bFinished)
{
	if (bFinished)
	{
		SetVehicleRaceInputEnabled(false);
		return;
	}

	ApplyRaceInputStateToVehicle();
}

void AOnlineRacingMatchPlayerController::UpdateRaceCountdown()
{
	if (!RaceGameState.IsValid() || RaceGameState->GetRacePhase() != EOnlineRacingMatchPhase::Countdown)
	{
		return;
	}

	const double TimeRemaining = RaceGameState->GetCountdownTimeRemaining();
	if (TimeRemaining > 0.0)
	{
		if (IsValid(RaceCountdownWidget))
		{
			RaceCountdownWidget->ShowCountdown(TimeRemaining);
		}
		return;
	}

	SetVehicleRaceInputEnabled(true);
	PresentRaceStart();
}

void AOnlineRacingMatchPlayerController::PresentRaceStart()
{
	if (bRaceStartPresented)
	{
		return;
	}

	bRaceStartPresented = true;
	if (!IsValid(RaceCountdownWidget))
	{
		return;
	}

	RaceCountdownWidget->ShowRaceStarted();
	GetWorldTimerManager().SetTimer(
		RaceStartedDisplayTimer,
		this,
		&AOnlineRacingMatchPlayerController::HideCountdownWidget,
		RaceStartedDisplayDuration,
		false);
}

void AOnlineRacingMatchPlayerController::HideCountdownWidget()
{
	if (IsValid(RaceCountdownWidget))
	{
		RaceCountdownWidget->HideCountdown();
	}
}

void AOnlineRacingMatchPlayerController::HideRaceResults()
{
	if (IsValid(RaceResultsWidget))
	{
		RaceResultsWidget->HideResults();
	}
}

void AOnlineRacingMatchPlayerController::SetDebugWidgetVisible(const bool bVisible)
{
	if (!IsValid(DebugWidget))
	{
		return;
	}

	if (bVisible)
	{
		DebugWidget->SetVisibility(ESlateVisibility::Visible);
		return;
	}

	DebugWidget->SetVisibility(ESlateVisibility::Collapsed);
}

void AOnlineRacingMatchPlayerController::ShowRaceResults()
{
	if (IsValid(RaceResultsWidget) && RaceGameState.IsValid())
	{
		RaceResultsWidget->ShowResults(RaceGameState->GetRaceResults());
	}
}

void AOnlineRacingMatchPlayerController::SetVehicleRaceInputEnabled(const bool bEnabled)
{
	if (IsValid(VehiclePawn))
	{
		VehiclePawn->SetRaceInputEnabled(bEnabled);
	}
}

void AOnlineRacingMatchPlayerController::ApplyRaceInputStateToVehicle()
{
	if (!IsValid(VehiclePawn) || !RaceGameState.IsValid())
	{
		return;
	}

	if (RacePlayerState.IsValid() && RacePlayerState->HasFinishedRace())
	{
		VehiclePawn->SetRaceInputEnabled(false);
		return;
	}

	bool bEnableRaceInput = RaceGameState->GetRacePhase() == EOnlineRacingMatchPhase::Racing;
	if (RaceGameState->GetRacePhase() == EOnlineRacingMatchPhase::Countdown && bRaceStartPresented)
	{
		bEnableRaceInput = true;
	}

	VehiclePawn->SetRaceInputEnabled(bEnableRaceInput);
}

void AOnlineRacingMatchPlayerController::CacheVehiclePawn()
{
	AOnlineRacingPawn* const NewVehiclePawn = Cast<AOnlineRacingPawn>(GetPawn());
	if (VehiclePawn == NewVehiclePawn)
	{
		return;
	}

	if (IsValid(VehiclePawn))
	{
		VehiclePawn->OnDestroyed.RemoveDynamic(this, &AOnlineRacingMatchPlayerController::OnPawnDestroyed);
	}

	VehiclePawn = NewVehiclePawn;
	if (IsValid(VehiclePawn))
	{
		VehiclePawn->OnDestroyed.AddUniqueDynamic(this, &AOnlineRacingMatchPlayerController::OnPawnDestroyed);
		ApplyRaceInputStateToVehicle();
		return;
	}

	if (IsValid(GetPawn()))
	{
		UE_LOG(LogOnlineRacing, Warning, TEXT("[Role=%s][PlayerController] Pawn %s is not an OnlineRacing vehicle."), *UEnum::GetValueAsString(GetLocalRole()), *GetNameSafe(GetPawn()));
	}
}

void AOnlineRacingMatchPlayerController::OnPawnDestroyed(AActor* DestroyedPawn)
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

bool AOnlineRacingMatchPlayerController::ShouldUseTouchControls() const
{
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}
