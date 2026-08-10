// Copyright Epic Games, Inc. All Rights Reserved.

#include "OnlineRacingPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/GameModeBase.h"
#include "InputMappingContext.h"
#include "TimerManager.h"
#include "Widgets/Input/SVirtualJoystick.h"

#include "OnlineRacing.h"
#include "OnlineRacingPawn.h"
#include "OnlineRacingUI.h"
#include "Race/OnlineRacingMatchGameMode.h"
#include "Race/OnlineRacingMatchGameState.h"
#include "Race/OnlineRacingMatchPlayerState.h"
#include "UI/OnlineRacingDebugWidget.h"
#include "UI/OnlineRacingCountdownWidget.h"
#include "UI/OnlineRacingResultsWidget.h"

void AOnlineRacingPlayerController::BeginPlay()
{
	Super::BeginPlay();

	bAttachToPawn = true;
	CacheVehiclePawn();
	BindRaceGameState();
	BindRacePlayerState();
}

void AOnlineRacingPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(RaceStartedDisplayTimer);
	UnbindRaceGameState();
	UnbindRacePlayerState();
	Super::EndPlay(EndPlayReason);
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

void AOnlineRacingPlayerController::Tick(const float DeltaSeconds)
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

void AOnlineRacingPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	BindRacePlayerState();
	CacheVehiclePawn();
}

void AOnlineRacingPlayerController::OnRep_Pawn()
{
	Super::OnRep_Pawn();
	CacheVehiclePawn();
}

void AOnlineRacingPlayerController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	BindRacePlayerState();
}

void AOnlineRacingPlayerController::RequestVehicleRespawn()
{
	if (HasAuthority())
	{
		HandleVehicleRespawnRequest();
		return;
	}

	Server_RequestVehicleRespawn();
}

void AOnlineRacingPlayerController::Server_RequestVehicleRespawn_Implementation()
{
	HandleVehicleRespawnRequest();
}

void AOnlineRacingPlayerController::HandleVehicleRespawnRequest()
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

void AOnlineRacingPlayerController::BindRaceGameState()
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

	RacePhaseChangedHandle = RaceGameState->OnRacePhaseChanged().AddUObject(this, &AOnlineRacingPlayerController::HandleRacePhaseChanged);
	RaceResultsChangedHandle = RaceGameState->OnRaceResultsChanged().AddUObject(this, &AOnlineRacingPlayerController::HandleRaceResultsChanged);
	HandleRacePhaseChanged(RaceGameState->GetRacePhase());
}

void AOnlineRacingPlayerController::UnbindRaceGameState()
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

void AOnlineRacingPlayerController::BindRacePlayerState()
{
	AOnlineRacingMatchPlayerState* const NewRacePlayerState = GetPlayerState<AOnlineRacingMatchPlayerState>();
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
		&AOnlineRacingPlayerController::HandlePlayerFinishedChanged);
	HandlePlayerFinishedChanged(RacePlayerState->HasFinishedRace());
}

void AOnlineRacingPlayerController::UnbindRacePlayerState()
{
	if (RacePlayerState.IsValid() && RaceFinishedChangedHandle.IsValid())
	{
		RacePlayerState->OnRaceFinishedChanged().Remove(RaceFinishedChangedHandle);
	}

	RaceFinishedChangedHandle.Reset();
	RacePlayerState.Reset();
}

void AOnlineRacingPlayerController::HandleRacePhaseChanged(const EOnlineRacingMatchPhase NewRacePhase)
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

void AOnlineRacingPlayerController::HandleRaceResultsChanged(const TArray<FOnlineRacingMatchResult>&)
{
	if (!RaceGameState.IsValid() || RaceGameState->GetRacePhase() != EOnlineRacingMatchPhase::Finished)
	{
		return;
	}

	ShowRaceResults();
}

void AOnlineRacingPlayerController::HandlePlayerFinishedChanged(const bool bFinished)
{
	if (bFinished)
	{
		SetVehicleRaceInputEnabled(false);
		return;
	}

	ApplyRaceInputStateToVehicle();
}

void AOnlineRacingPlayerController::UpdateRaceCountdown()
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

void AOnlineRacingPlayerController::PresentRaceStart()
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
		&AOnlineRacingPlayerController::HideCountdownWidget,
		RaceStartedDisplayDuration,
		false);
}

void AOnlineRacingPlayerController::HideCountdownWidget()
{
	if (IsValid(RaceCountdownWidget))
	{
		RaceCountdownWidget->HideCountdown();
	}
}

void AOnlineRacingPlayerController::HideRaceResults()
{
	if (IsValid(RaceResultsWidget))
	{
		RaceResultsWidget->HideResults();
	}
}

void AOnlineRacingPlayerController::SetDebugWidgetVisible(const bool bVisible)
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

void AOnlineRacingPlayerController::ShowRaceResults()
{
	if (IsValid(RaceResultsWidget) && RaceGameState.IsValid())
	{
		RaceResultsWidget->ShowResults(RaceGameState->GetRaceResults());
	}
}

void AOnlineRacingPlayerController::SetVehicleRaceInputEnabled(const bool bEnabled)
{
	if (IsValid(VehiclePawn))
	{
		VehiclePawn->SetRaceInputEnabled(bEnabled);
	}
}

void AOnlineRacingPlayerController::ApplyRaceInputStateToVehicle()
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
		ApplyRaceInputStateToVehicle();
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
