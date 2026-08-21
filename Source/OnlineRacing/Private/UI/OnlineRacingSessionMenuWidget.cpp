// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/OnlineRacingSessionMenuWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"

#include "OnlineRacing.h"

void UOnlineRacingSessionMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	const UGameInstance* CurrentGameInstance = GetGameInstance();
	if (!IsValid(CurrentGameInstance))
	{
		return;
	}

	SessionSubsystem = CurrentGameInstance->GetSubsystem<UOnlineRacingSessionSubsystem>();
	if (!IsValid(SessionSubsystem))
	{
		UE_LOG(LogOnlineRacing, Error, TEXT("[Local][SessionMenuWidget] Session subsystem is unavailable. Widget=%s"), *GetNameSafe(this));
		SetStatusText(NSLOCTEXT("OnlineRacing", "SessionsUnavailable", "Online sessions are unavailable."));
		UpdateControls();
		return;
	}

	SessionSubsystem->OnHostSessionComplete.AddUniqueDynamic(this, &ThisClass::HandleHostSessionComplete);
	SessionSubsystem->OnFindSessionsComplete.AddUniqueDynamic(this, &ThisClass::HandleFindSessionsComplete);
	SessionSubsystem->OnJoinSessionComplete.AddUniqueDynamic(this, &ThisClass::HandleJoinSessionComplete);
	SessionSubsystem->OnSessionError.AddUniqueDynamic(this, &ThisClass::HandleSessionError);

	if (IsValid(Button_Host))
	{
		Button_Host->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleHostButtonClicked);
	}

	if (IsValid(Button_Refresh))
	{
		Button_Refresh->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleRefreshButtonClicked);
	}

	if (IsValid(Button_Join))
	{
		Button_Join->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleJoinButtonClicked);
	}

	SetStatusText(FText::Format(
		NSLOCTEXT("OnlineRacing", "SessionMenuIdle", "Backend: {0}. Host a game or search for sessions."),
		FText::FromName(SessionSubsystem->GetActiveSubsystemName())));

	if (IsValid(Text_SessionResult))
	{
		Text_SessionResult->SetText(NSLOCTEXT("OnlineRacing", "NoSessionSelected", "No session selected."));
	}

	UpdateControls();
}

void UOnlineRacingSessionMenuWidget::NativeDestruct()
{
	if (IsValid(SessionSubsystem))
	{
		SessionSubsystem->OnHostSessionComplete.RemoveDynamic(this, &ThisClass::HandleHostSessionComplete);
		SessionSubsystem->OnFindSessionsComplete.RemoveDynamic(this, &ThisClass::HandleFindSessionsComplete);
		SessionSubsystem->OnJoinSessionComplete.RemoveDynamic(this, &ThisClass::HandleJoinSessionComplete);
		SessionSubsystem->OnSessionError.RemoveDynamic(this, &ThisClass::HandleSessionError);
	}

	if (IsValid(Button_Host))
	{
		Button_Host->OnClicked.RemoveDynamic(this, &ThisClass::HandleHostButtonClicked);
	}

	if (IsValid(Button_Refresh))
	{
		Button_Refresh->OnClicked.RemoveDynamic(this, &ThisClass::HandleRefreshButtonClicked);
	}

	if (IsValid(Button_Join))
	{
		Button_Join->OnClicked.RemoveDynamic(this, &ThisClass::HandleJoinButtonClicked);
	}

	Super::NativeDestruct();
}

void UOnlineRacingSessionMenuWidget::HandleHostButtonClicked()
{
	if (!IsValid(SessionSubsystem) || bOperationInProgress)
	{
		return;
	}

	SelectedResultIndex = INDEX_NONE;
	SetOperationInProgress(true);
	SetStatusText(NSLOCTEXT("OnlineRacing", "CreatingSession", "Creating session..."));
	SessionSubsystem->HostSession(MaxPublicConnections);
}

void UOnlineRacingSessionMenuWidget::HandleRefreshButtonClicked()
{
	if (!IsValid(SessionSubsystem) || bOperationInProgress)
	{
		return;
	}

	SelectedResultIndex = INDEX_NONE;
	SetOperationInProgress(true);
	SetStatusText(NSLOCTEXT("OnlineRacing", "SearchingSessions", "Searching for sessions..."));
	if (IsValid(Text_SessionResult))
	{
		Text_SessionResult->SetText(FText::GetEmpty());
	}
	SessionSubsystem->FindSessions(MaxSearchResults);
}

void UOnlineRacingSessionMenuWidget::HandleJoinButtonClicked()
{
	if (!IsValid(SessionSubsystem) || bOperationInProgress || SelectedResultIndex == INDEX_NONE)
	{
		return;
	}

	SetOperationInProgress(true);
	SetStatusText(NSLOCTEXT("OnlineRacing", "JoiningSession", "Joining session..."));
	SessionSubsystem->JoinSession(SelectedResultIndex);
}

void UOnlineRacingSessionMenuWidget::HandleHostSessionComplete(const bool bWasSuccessful)
{
	SetOperationInProgress(false);
	if (bWasSuccessful)
	{
		SetStatusText(NSLOCTEXT("OnlineRacing", "OpeningLobby", "Opening lobby..."));
	}
}

void UOnlineRacingSessionMenuWidget::HandleFindSessionsComplete(const TArray<FOnlineRacingSessionInfo>& Sessions, const bool bWasSuccessful)
{
	SetOperationInProgress(false);
	SelectedResultIndex = INDEX_NONE;

	if (!bWasSuccessful)
	{
		UpdateControls();
		return;
	}

	if (Sessions.IsEmpty())
	{
		SetStatusText(NSLOCTEXT("OnlineRacing", "NoSessionsFound", "No sessions found."));
		if (IsValid(Text_SessionResult))
		{
			Text_SessionResult->SetText(NSLOCTEXT("OnlineRacing", "NoAvailableSessions", "No available sessions."));
		}
		UpdateControls();
		return;
	}

	const FOnlineRacingSessionInfo& SessionInfo = Sessions[0];
	SelectedResultIndex = SessionInfo.ResultIndex;
	SetStatusText(FText::Format(
		NSLOCTEXT("OnlineRacing", "SessionsFound", "Found {0} session(s)."),
		FText::AsNumber(Sessions.Num())));

	if (IsValid(Text_SessionResult))
	{
		Text_SessionResult->SetText(FText::Format(
			NSLOCTEXT("OnlineRacing", "SessionDetails", "Host: {0} | Players: {1}/{2} | Ping: {3} ms"),
			FText::FromString(SessionInfo.OwningPlayerName),
			FText::AsNumber(SessionInfo.CurrentPlayers),
			FText::AsNumber(SessionInfo.MaxPlayers),
			FText::AsNumber(SessionInfo.PingInMs)));
	}

	UpdateControls();
}

void UOnlineRacingSessionMenuWidget::HandleJoinSessionComplete(const bool bWasSuccessful)
{
	SetOperationInProgress(false);
	if (bWasSuccessful)
	{
		SetStatusText(NSLOCTEXT("OnlineRacing", "ConnectingToLobby", "Connecting to lobby..."));
	}
}

void UOnlineRacingSessionMenuWidget::HandleSessionError(const FText ErrorMessage) const
{
	SetStatusText(ErrorMessage);
}

void UOnlineRacingSessionMenuWidget::SetOperationInProgress(const bool bNewOperationInProgress)
{
	bOperationInProgress = bNewOperationInProgress;
	UpdateControls();
}

void UOnlineRacingSessionMenuWidget::SetStatusText(const FText& StatusText) const
{
	if (IsValid(Text_Status))
	{
		Text_Status->SetText(StatusText);
	}
}

void UOnlineRacingSessionMenuWidget::UpdateControls() const
{
	const bool bSessionSubsystemAvailable = IsValid(SessionSubsystem);
	if (IsValid(Button_Host))
	{
		Button_Host->SetIsEnabled(bSessionSubsystemAvailable && !bOperationInProgress);
	}

	if (IsValid(Button_Refresh))
	{
		Button_Refresh->SetIsEnabled(bSessionSubsystemAvailable && !bOperationInProgress);
	}

	if (IsValid(Button_Join))
	{
		Button_Join->SetIsEnabled(bSessionSubsystemAvailable && !bOperationInProgress && SelectedResultIndex != INDEX_NONE);
	}
}


