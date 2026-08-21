// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/OnlineRacingLobbyWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"

#include "Framework/GameStates/OnlineRacingLobbyGameState.h"
#include "OnlineRacing.h"
#include "Framework/PlayerControllers/OnlineRacingLobbyPlayerController.h"
#include "Framework/PlayerStates/OnlineRacingPlayerState.h"

void UOnlineRacingLobbyWidget::NativeConstruct()
{
	Super::NativeConstruct();

	const UWorld* CurrentWorld = GetWorld();
	const APlayerController* OwningPlayerController = GetOwningPlayer();
	if (!IsValid(CurrentWorld) || !IsValid(OwningPlayerController))
	{
		return;
	}

	OnlineRacingLobbyGameState = CurrentWorld->GetGameState<AOnlineRacingLobbyGameState>();
	OwningOnlineRacingPlayerState = OwningPlayerController->GetPlayerState<AOnlineRacingPlayerState>();
	if (!IsValid(OnlineRacingLobbyGameState) || !IsValid(OwningOnlineRacingPlayerState))
	{
		UE_LOG(LogOnlineRacing, Warning,
			TEXT("[Local][LobbyWidget] Lobby state binding failed. Widget=%s GameState=%s PlayerState=%s"),
			*GetNameSafe(this),
			*GetNameSafe(OnlineRacingLobbyGameState),
			*GetNameSafe(OwningOnlineRacingPlayerState));
		return;
	}

	OnlineRacingLobbyGameState->OnLobbyStateChanged.AddUniqueDynamic(this, &ThisClass::HandleLobbyStateChanged);
	OwningOnlineRacingPlayerState->OnLobbyPlayerStateChanged.AddUniqueDynamic(this, &ThisClass::HandleLobbyPlayerStateChanged);

	if (IsValid(Button_Ready))
	{
		Button_Ready->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleReadyButtonClicked);
	}

	if (IsValid(Button_Start))
	{
		Button_Start->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleStartButtonClicked);
	}

	HandleLobbyStateChanged(
		OnlineRacingLobbyGameState->GetReadyPlayerCount(),
		OnlineRacingLobbyGameState->GetTotalPlayerCount(),
		OnlineRacingLobbyGameState->CanStartMatch(),
		OnlineRacingLobbyGameState->IsTravelingToMatch());
}

void UOnlineRacingLobbyWidget::NativeDestruct()
{
	if (IsValid(OnlineRacingLobbyGameState))
	{
		OnlineRacingLobbyGameState->OnLobbyStateChanged.RemoveDynamic(this, &ThisClass::HandleLobbyStateChanged);
	}

	if (IsValid(OwningOnlineRacingPlayerState))
	{
		OwningOnlineRacingPlayerState->OnLobbyPlayerStateChanged.RemoveDynamic(this, &ThisClass::HandleLobbyPlayerStateChanged);
	}

	if (IsValid(Button_Ready))
	{
		Button_Ready->OnClicked.RemoveDynamic(this, &ThisClass::HandleReadyButtonClicked);
	}

	if (IsValid(Button_Start))
	{
		Button_Start->OnClicked.RemoveDynamic(this, &ThisClass::HandleStartButtonClicked);
	}

	Super::NativeDestruct();
}

void UOnlineRacingLobbyWidget::HandleLobbyStateChanged(const int32 ReadyPlayerCount, const int32 TotalPlayerCount, const bool bCanStartMatch, const bool bIsTravelingToMatch)
{
	if (!IsValid(OwningOnlineRacingPlayerState))
	{
		return;
	}

	bLocalPlayerReady = OwningOnlineRacingPlayerState->IsLobbyReady();
	const bool bLocalPlayerHost = OwningOnlineRacingPlayerState->IsLobbyHost();

	if (IsValid(Text_PlayerCount))
	{
		if (bIsTravelingToMatch)
		{
			Text_PlayerCount->SetVisibility(ESlateVisibility::Collapsed);
		}
		else
		{
			const FText PlayerCountText = FText::Format(
				NSLOCTEXT("OnlineRacing", "LobbyReadyPlayerCount", "Ready: {0} / {1}"),
				FText::AsNumber(ReadyPlayerCount),
				FText::AsNumber(TotalPlayerCount));
			Text_PlayerCount->SetText(PlayerCountText);
			Text_PlayerCount->SetVisibility(ESlateVisibility::Visible);
		}
	}

	FText ReadyStatusText = NSLOCTEXT("OnlineRacing", "LobbyNotReadyStatus", "Status: Not Ready");
	FText ReadyButtonText = NSLOCTEXT("OnlineRacing", "LobbyReadyButton", "Ready");
	if (bIsTravelingToMatch)
	{
		ReadyStatusText = NSLOCTEXT("OnlineRacing", "LobbyConnectingStatus", "Connecting to the game...");
	}
	else if (bLocalPlayerReady)
	{
		ReadyStatusText = NSLOCTEXT("OnlineRacing", "LobbyReadyStatus", "Status: Ready");
		ReadyButtonText = NSLOCTEXT("OnlineRacing", "LobbyNotReadyButton", "Not Ready");
	}

	if (IsValid(Text_ReadyStatus))
	{
		Text_ReadyStatus->SetText(ReadyStatusText);
	}

	if (IsValid(Text_ReadyButton))
	{
		Text_ReadyButton->SetText(ReadyButtonText);
	}

	if (IsValid(Button_Ready))
	{
		Button_Ready->SetIsEnabled(!bIsTravelingToMatch);
	}

	if (IsValid(Button_Start))
	{
		ESlateVisibility StartButtonVisibility = ESlateVisibility::Collapsed;
		if (bLocalPlayerHost)
		{
			StartButtonVisibility = ESlateVisibility::Visible;
		}

		Button_Start->SetVisibility(StartButtonVisibility);
		Button_Start->SetIsEnabled(bLocalPlayerHost && bCanStartMatch && !bIsTravelingToMatch);
	}

	BP_UpdateLobbyState(
		ReadyPlayerCount,
		TotalPlayerCount,
		bCanStartMatch,
		bLocalPlayerReady,
		bLocalPlayerHost);
}

void UOnlineRacingLobbyWidget::HandleLobbyPlayerStateChanged(const bool, const bool)
{
	if (!IsValid(OnlineRacingLobbyGameState))
	{
		return;
	}

	HandleLobbyStateChanged(
		OnlineRacingLobbyGameState->GetReadyPlayerCount(),
		OnlineRacingLobbyGameState->GetTotalPlayerCount(),
		OnlineRacingLobbyGameState->CanStartMatch(),
		OnlineRacingLobbyGameState->IsTravelingToMatch());
}

void UOnlineRacingLobbyWidget::HandleReadyButtonClicked() const
{
	AOnlineRacingLobbyPlayerController* LobbyPlayerController = Cast<AOnlineRacingLobbyPlayerController>(GetOwningPlayer());
	if (!IsValid(LobbyPlayerController))
	{
		UE_LOG(LogOnlineRacing, Warning, TEXT("[Local][LobbyWidget] Ready button ignored: owning lobby controller unavailable. Widget=%s"), *GetNameSafe(this));
		return;
	}

	LobbyPlayerController->SetLobbyReady(!bLocalPlayerReady);
}

void UOnlineRacingLobbyWidget::HandleStartButtonClicked() const
{
	AOnlineRacingLobbyPlayerController* LobbyPlayerController = Cast<AOnlineRacingLobbyPlayerController>(GetOwningPlayer());
	if (!IsValid(LobbyPlayerController))
	{
		UE_LOG(LogOnlineRacing, Warning, TEXT("[Local][LobbyWidget] Start button ignored: owning lobby controller unavailable. Widget=%s"), *GetNameSafe(this));
		return;
	}

	LobbyPlayerController->RequestLobbyMatchStart();
}



