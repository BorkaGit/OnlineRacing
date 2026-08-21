// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/PlayerControllers/OnlineRacingLobbyPlayerController.h"

#include "Framework/GameModes/OnlineRacingLobbyGameMode.h"
#include "OnlineRacing.h"
#include "Framework/PlayerStates/OnlineRacingPlayerState.h"
#include "UI/OnlineRacingLobbyWidget.h"

void AOnlineRacingLobbyPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalPlayerController())
	{
		return;
	}

	ConfigureLobbyInputMode();
	TryCreateLobbyWidget();
}

void AOnlineRacingLobbyPlayerController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	RemoveLobbyWidget();
	TryCreateLobbyWidget();
}

void AOnlineRacingLobbyPlayerController::SetLobbyReady(const bool bIsReady)
{
	if (!IsLocalController())
	{
		return;
	}

	Server_SetLobbyReady(bIsReady);
}

void AOnlineRacingLobbyPlayerController::RequestLobbyMatchStart()
{
	if (!IsLocalController())
	{
		return;
	}

	Server_RequestLobbyMatchStart();
}

void AOnlineRacingLobbyPlayerController::ConfigureLobbyInputMode()
{
	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);
	SetShowMouseCursor(true);
}

void AOnlineRacingLobbyPlayerController::RemoveLobbyWidget()
{
	if (!IsValid(LobbyWidget))
	{
		return;
	}

	LobbyWidget->RemoveFromParent();
	LobbyWidget = nullptr;
}

void AOnlineRacingLobbyPlayerController::TryCreateLobbyWidget()
{
	if (!IsLocalPlayerController() || IsValid(LobbyWidget) || !IsValid(PlayerState.Get()))
	{
		return;
	}

	if (!IsValid(LobbyWidgetClass))
	{
		UE_LOG(LogOnlineRacing, Error, TEXT("[Local][LobbyPlayerController] Lobby widget class is not configured. Controller=%s"), *GetNameSafe(this));
		return;
	}

	LobbyWidget = CreateWidget<UOnlineRacingLobbyWidget>(this, LobbyWidgetClass);
	if (!IsValid(LobbyWidget))
	{
		UE_LOG(LogOnlineRacing, Error, TEXT("[Local][LobbyPlayerController] Failed to create lobby widget. Controller=%s"), *GetNameSafe(this));
		return;
	}

	LobbyWidget->AddToPlayerScreen(0);
}

void AOnlineRacingLobbyPlayerController::Server_SetLobbyReady_Implementation(const bool bIsReady)
{
	const UWorld* CurrentWorld = GetWorld();
	if (!IsValid(CurrentWorld))
	{
		return;
	}

	AOnlineRacingLobbyGameMode* LobbyGameMode = CurrentWorld->GetAuthGameMode<AOnlineRacingLobbyGameMode>();
	AOnlineRacingPlayerState* OnlineRacingPlayerState = GetPlayerState<AOnlineRacingPlayerState>();
	if (!IsValid(LobbyGameMode) || !IsValid(OnlineRacingPlayerState))
	{
		UE_LOG(LogOnlineRacing, Warning, TEXT("[Server][LobbyPlayerController] Ready request rejected: lobby framework unavailable. Controller=%s"), *GetNameSafe(this));
		return;
	}

	LobbyGameMode->SetPlayerReady(OnlineRacingPlayerState, bIsReady);
}

void AOnlineRacingLobbyPlayerController::Server_RequestLobbyMatchStart_Implementation()
{
	const UWorld* CurrentWorld = GetWorld();
	if (!IsValid(CurrentWorld))
	{
		return;
	}

	AOnlineRacingLobbyGameMode* LobbyGameMode = CurrentWorld->GetAuthGameMode<AOnlineRacingLobbyGameMode>();
	if (!IsValid(LobbyGameMode))
	{
		UE_LOG(LogOnlineRacing, Warning, TEXT("[Server][LobbyPlayerController] Start request rejected: lobby framework unavailable. Controller=%s"), *GetNameSafe(this));
		return;
	}

	LobbyGameMode->HandleStartMatchRequest(this);
}