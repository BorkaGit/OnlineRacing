// Fill out your copyright notice in the Description page of Project Settings.


#include "Online/OnlineRacingSessionSubsystem.h"

#include "Engine/EngineBaseTypes.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"
#include "Online/OnlineSessionNames.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"

#include "OnlineRacing.h"

namespace OnlineRacingSession
{
	const FName GameIdentifierKey(TEXT("OnlineRacing_GAME"));
	const FName MapNameKey(TEXT("MAPNAME"));
	const FString GameIdentifier(TEXT("OnlineRacing"));
}

UOnlineRacingSessionSubsystem::UOnlineRacingSessionSubsystem()
	: LobbyMapPath(TEXT("/Game/Maps/Lvl_Lobby.Lvl_Lobby"))
{
}

void UOnlineRacingSessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (!RefreshSessionInterface())
	{
		UE_LOG(LogOnlineRacing, Error, TEXT("[Local][SessionSubsystem] Initialization failed: OnlineSubsystem is unavailable."));
		return;
	}

	UE_LOG(LogOnlineRacing, Log,
		TEXT("[Local][SessionSubsystem] Initialized. Backend=%s LobbyMap=%s"),
		*ActiveSubsystemName.ToString(),
		*LobbyMapPath.ToString());
}

void UOnlineRacingSessionSubsystem::Deinitialize()
{
	ClearOnlineDelegates();
	SessionSearch.Reset();
	SessionInterface.Reset();
	ActiveSubsystemName = NAME_None;
	CurrentOperation = ESessionOperation::None;

	Super::Deinitialize();
}

void UOnlineRacingSessionSubsystem::HostSession(const int32 MaxPublicConnections)
{
	if (!CanBeginOperation(TEXT("HostSession")))
	{
		OnHostSessionComplete.Broadcast(false);
		return;
	}

	const IOnlineSessionPtr PinnedSessionInterface = SessionInterface.Pin();
	if (!PinnedSessionInterface.IsValid())
	{
		ReportError(NSLOCTEXT("OnlineRacing", "SessionInterfaceExpiredForHost", "Online sessions are unavailable."));
		OnHostSessionComplete.Broadcast(false);
		return;
	}

	const FString LobbyMapPackageName = LobbyMapPath.GetLongPackageName();
	if (!FPackageName::IsValidLongPackageName(LobbyMapPackageName))
	{
		ReportError(NSLOCTEXT("OnlineRacing", "InvalidLobbyMapForSession", "The lobby map is not configured correctly."));
		OnHostSessionComplete.Broadcast(false);
		return;
	}

	PendingMaxPublicConnections = FMath::Max(1, MaxPublicConnections);
	if (PinnedSessionInterface->GetNamedSession(NAME_GameSession) != nullptr)
	{
		if (!TryDestroySession(ESessionOperation::DestroyingForRecreate))
		{
			OnHostSessionComplete.Broadcast(false);
		}
		return;
	}

	FOnlineSessionSettings SessionSettings;
	SessionSettings.bAllowInvites = true;
	SessionSettings.bAllowJoinInProgress = true;
	SessionSettings.bAllowJoinViaPresence = !IsUsingLanBackend();
	SessionSettings.bAllowJoinViaPresenceFriendsOnly = false;
	SessionSettings.bAntiCheatProtected = false;
	SessionSettings.bIsDedicated = false;
	SessionSettings.bIsLANMatch = IsUsingLanBackend();
	SessionSettings.bShouldAdvertise = true;
	SessionSettings.bUseLobbiesIfAvailable = !IsUsingLanBackend();
	SessionSettings.bUsesPresence = !IsUsingLanBackend();
	SessionSettings.NumPrivateConnections = 0;
	SessionSettings.NumPublicConnections = PendingMaxPublicConnections;
	SessionSettings.Set(
		OnlineRacingSession::GameIdentifierKey,
		OnlineRacingSession::GameIdentifier,
		EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);
	SessionSettings.Set(
		OnlineRacingSession::MapNameKey,
		LobbyMapPackageName,
		EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	CurrentOperation = ESessionOperation::Creating;
	CreateSessionCompleteDelegateHandle = PinnedSessionInterface->AddOnCreateSessionCompleteDelegate_Handle(
		FOnCreateSessionCompleteDelegate::CreateUObject(this, &ThisClass::HandleCreateSessionComplete));

	UE_LOG(LogOnlineRacing, Log,
		TEXT("[Local][SessionSubsystem] Creating session. Backend=%s LAN=%d PublicConnections=%d Map=%s"),
		*ActiveSubsystemName.ToString(),
		SessionSettings.bIsLANMatch,
		PendingMaxPublicConnections,
		*LobbyMapPackageName);

	if (!PinnedSessionInterface->CreateSession(0, NAME_GameSession, SessionSettings))
	{
		PinnedSessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
		CreateSessionCompleteDelegateHandle.Reset();
		CurrentOperation = ESessionOperation::None;
		ReportError(NSLOCTEXT("OnlineRacing", "CreateSessionFailedToStart", "The session could not be created."));
		OnHostSessionComplete.Broadcast(false);
	}
}

void UOnlineRacingSessionSubsystem::FindSessions(const int32 MaxSearchResults)
{
	if (!CanBeginOperation(TEXT("FindSessions")))
	{
		OnFindSessionsComplete.Broadcast(TArray<FOnlineRacingSessionInfo>(), false);
		return;
	}

	const IOnlineSessionPtr PinnedSessionInterface = SessionInterface.Pin();
	if (!PinnedSessionInterface.IsValid())
	{
		ReportError(NSLOCTEXT("OnlineRacing", "SessionInterfaceExpiredForSearch", "Online sessions are unavailable."));
		OnFindSessionsComplete.Broadcast(TArray<FOnlineRacingSessionInfo>(), false);
		return;
	}

	SessionSearch = MakeShared<FOnlineSessionSearch>();
	SessionSearch->bIsLanQuery = IsUsingLanBackend();
	SessionSearch->MaxSearchResults = FMath::Max(1, MaxSearchResults);
	SessionSearch->PingBucketSize = 50;
	if (!IsUsingLanBackend())
	{
		SessionSearch->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);
	}
	SessionSearch->QuerySettings.Set(
		OnlineRacingSession::GameIdentifierKey,
		OnlineRacingSession::GameIdentifier,
		EOnlineComparisonOp::Equals);

	CurrentOperation = ESessionOperation::Finding;
	FindSessionsCompleteDelegateHandle = PinnedSessionInterface->AddOnFindSessionsCompleteDelegate_Handle(
		FOnFindSessionsCompleteDelegate::CreateUObject(this, &ThisClass::HandleFindSessionsComplete));

	UE_LOG(LogOnlineRacing, Log,
		TEXT("[Local][SessionSubsystem] Searching for sessions. Backend=%s LAN=%d MaxResults=%d"),
		*ActiveSubsystemName.ToString(),
		SessionSearch->bIsLanQuery,
		SessionSearch->MaxSearchResults);

	if (!PinnedSessionInterface->FindSessions(0, SessionSearch.ToSharedRef()))
	{
		PinnedSessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
		FindSessionsCompleteDelegateHandle.Reset();
		CurrentOperation = ESessionOperation::None;
		ReportError(NSLOCTEXT("OnlineRacing", "FindSessionsFailedToStart", "The session search could not be started."));
		OnFindSessionsComplete.Broadcast(TArray<FOnlineRacingSessionInfo>(), false);
	}
}

void UOnlineRacingSessionSubsystem::JoinSession(const int32 ResultIndex)
{
	if (!CanBeginOperation(TEXT("JoinSession")))
	{
		OnJoinSessionComplete.Broadcast(false);
		return;
	}

	const IOnlineSessionPtr PinnedSessionInterface = SessionInterface.Pin();
	if (!PinnedSessionInterface.IsValid())
	{
		ReportError(NSLOCTEXT("OnlineRacing", "SessionInterfaceExpiredForJoin", "Online sessions are unavailable."));
		OnJoinSessionComplete.Broadcast(false);
		return;
	}

	if (!SessionSearch.IsValid() || !SessionSearch->SearchResults.IsValidIndex(ResultIndex))
	{
		ReportError(NSLOCTEXT("OnlineRacing", "InvalidSessionResult", "The selected session is no longer available."));
		OnJoinSessionComplete.Broadcast(false);
		return;
	}

	const FOnlineSessionSearchResult& SearchResult = SessionSearch->SearchResults[ResultIndex];
	if (!SearchResult.IsValid())
	{
		ReportError(NSLOCTEXT("OnlineRacing", "InvalidSessionData", "The selected session returned invalid connection data."));
		OnJoinSessionComplete.Broadcast(false);
		return;
	}

	if (PinnedSessionInterface->GetNamedSession(NAME_GameSession) != nullptr)
	{
		PendingJoinResultIndex = ResultIndex;
		if (!TryDestroySession(ESessionOperation::DestroyingForJoin))
		{
			OnJoinSessionComplete.Broadcast(false);
		}
		return;
	}

	CurrentOperation = ESessionOperation::Joining;
	JoinSessionCompleteDelegateHandle = PinnedSessionInterface->AddOnJoinSessionCompleteDelegate_Handle(
		FOnJoinSessionCompleteDelegate::CreateUObject(this, &ThisClass::HandleJoinSessionComplete));

	UE_LOG(LogOnlineRacing, Log,
		TEXT("[Local][SessionSubsystem] Joining session. Backend=%s ResultIndex=%d Owner=%s Ping=%d"),
		*ActiveSubsystemName.ToString(),
		ResultIndex,
		*SearchResult.Session.OwningUserName,
		SearchResult.PingInMs);

	if (!PinnedSessionInterface->JoinSession(0, NAME_GameSession, SearchResult))
	{
		PinnedSessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
		JoinSessionCompleteDelegateHandle.Reset();
		CurrentOperation = ESessionOperation::None;
		ReportError(NSLOCTEXT("OnlineRacing", "JoinSessionFailedToStart", "The session could not be joined."));
		OnJoinSessionComplete.Broadcast(false);
	}
}

void UOnlineRacingSessionSubsystem::DestroySession()
{
	if (!CanBeginOperation(TEXT("DestroySession")))
	{
		OnDestroySessionComplete.Broadcast(false);
		return;
	}

	const IOnlineSessionPtr PinnedSessionInterface = SessionInterface.Pin();
	if (!PinnedSessionInterface.IsValid())
	{
		ReportError(NSLOCTEXT("OnlineRacing", "SessionInterfaceExpiredForDestroy", "Online sessions are unavailable."));
		OnDestroySessionComplete.Broadcast(false);
		return;
	}

	if (PinnedSessionInterface->GetNamedSession(NAME_GameSession) == nullptr)
	{
		OnDestroySessionComplete.Broadcast(true);
		return;
	}

	if (!TryDestroySession(ESessionOperation::Destroying))
	{
		OnDestroySessionComplete.Broadcast(false);
	}
}

bool UOnlineRacingSessionSubsystem::HasActiveSession() const
{
	const IOnlineSessionPtr PinnedSessionInterface = SessionInterface.Pin();
	return PinnedSessionInterface.IsValid() && PinnedSessionInterface->GetNamedSession(NAME_GameSession) != nullptr;
}

FName UOnlineRacingSessionSubsystem::GetActiveSubsystemName() const
{
	return ActiveSubsystemName;
}

bool UOnlineRacingSessionSubsystem::IsUsingLanBackend() const
{
	return ActiveSubsystemName == NULL_SUBSYSTEM;
}

void UOnlineRacingSessionSubsystem::HandleCreateSessionComplete(const FName SessionName, const bool bWasSuccessful)
{
	const IOnlineSessionPtr PinnedSessionInterface = SessionInterface.Pin();
	if (PinnedSessionInterface.IsValid())
	{
		PinnedSessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
	}
	CreateSessionCompleteDelegateHandle.Reset();
	CurrentOperation = ESessionOperation::None;

	if (!bWasSuccessful)
	{
		UE_LOG(LogOnlineRacing, Error,
			TEXT("[Local][SessionSubsystem] Session creation failed. Backend=%s Session=%s"),
			*ActiveSubsystemName.ToString(),
			*SessionName.ToString());
		ReportError(NSLOCTEXT("OnlineRacing", "CreateSessionFailed", "Failed to create the session."));
		OnHostSessionComplete.Broadcast(false);
		return;
	}

	const FString LobbyMapPackageName = LobbyMapPath.GetLongPackageName();
	UE_LOG(LogOnlineRacing, Log,
		TEXT("[Host][SessionSubsystem] Session created. Backend=%s Session=%s Map=%s"),
		*ActiveSubsystemName.ToString(),
		*SessionName.ToString(),
		*LobbyMapPackageName);
	OnHostSessionComplete.Broadcast(true);

	FString ListenOptions(TEXT("listen"));
	if (IsUsingLanBackend())
	{
		ListenOptions += TEXT("?bIsLanMatch");
	}
	UGameplayStatics::OpenLevel(this, FName(*LobbyMapPackageName), true, *ListenOptions);
}

void UOnlineRacingSessionSubsystem::HandleFindSessionsComplete(const bool bWasSuccessful)
{
	const IOnlineSessionPtr PinnedSessionInterface = SessionInterface.Pin();
	if (PinnedSessionInterface.IsValid())
	{
		PinnedSessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
	}
	FindSessionsCompleteDelegateHandle.Reset();
	CurrentOperation = ESessionOperation::None;

	TArray<FOnlineRacingSessionInfo> SessionInfos;
	if (bWasSuccessful && SessionSearch.IsValid())
	{
		for (int32 ResultIndex = 0; ResultIndex < SessionSearch->SearchResults.Num(); ++ResultIndex)
		{
			const FOnlineSessionSearchResult& SearchResult = SessionSearch->SearchResults[ResultIndex];
			if (!SearchResult.IsValid())
			{
				continue;
			}

			FOnlineRacingSessionInfo& SessionInfo = SessionInfos.AddDefaulted_GetRef();
			SessionInfo.ResultIndex = ResultIndex;
			SessionInfo.OwningPlayerName = SearchResult.Session.OwningUserName;
			SessionInfo.MaxPlayers = SearchResult.Session.SessionSettings.NumPublicConnections;
			SessionInfo.CurrentPlayers = FMath::Max(0, SessionInfo.MaxPlayers - SearchResult.Session.NumOpenPublicConnections);
			SessionInfo.PingInMs = FMath::Max(0, SearchResult.PingInMs);
		}
	}

	FString SuccessState = TEXT("false");
	if (bWasSuccessful)
	{
		SuccessState = TEXT("true");
	}

	UE_LOG(LogOnlineRacing, Log,
		TEXT("[Local][SessionSubsystem] Session search completed. Backend=%s Success=%s Results=%d"),
		*ActiveSubsystemName.ToString(),
		*SuccessState,
		SessionInfos.Num());
	OnFindSessionsComplete.Broadcast(SessionInfos, bWasSuccessful);
}

void UOnlineRacingSessionSubsystem::HandleJoinSessionComplete(const FName SessionName, const EOnJoinSessionCompleteResult::Type Result)
{
	const IOnlineSessionPtr PinnedSessionInterface = SessionInterface.Pin();
	if (PinnedSessionInterface.IsValid())
	{
		PinnedSessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
	}
	JoinSessionCompleteDelegateHandle.Reset();
	CurrentOperation = ESessionOperation::None;

	if (Result != EOnJoinSessionCompleteResult::Success || !PinnedSessionInterface.IsValid())
	{
		UE_LOG(LogOnlineRacing, Error,
			TEXT("[Local][SessionSubsystem] Session join failed. Backend=%s Session=%s Result=%d"),
			*ActiveSubsystemName.ToString(),
			*SessionName.ToString(),
			static_cast<int32>(Result));
		ReportError(NSLOCTEXT("OnlineRacing", "JoinSessionFailed", "Failed to join the session."));
		OnJoinSessionComplete.Broadcast(false);
		return;
	}

	FString ConnectString;
	if (!PinnedSessionInterface->GetResolvedConnectString(SessionName, ConnectString) || ConnectString.IsEmpty())
	{
		ReportError(NSLOCTEXT("OnlineRacing", "ResolveSessionFailed", "The session address could not be resolved."));
		if (!TryDestroySession(ESessionOperation::DestroyingAfterFailedJoin))
		{
			OnJoinSessionComplete.Broadcast(false);
		}
		return;
	}

	const FURL ConnectUrl(nullptr, *ConnectString, TRAVEL_Absolute);
	if (!ConnectUrl.Valid || ConnectUrl.Port <= 0)
	{
		UE_LOG(LogOnlineRacing, Error,
			TEXT("[Local][SessionSubsystem] Resolved session address is invalid. Backend=%s Session=%s Address=%s Port=%d"),
			*ActiveSubsystemName.ToString(),
			*SessionName.ToString(),
			*ConnectString,
			ConnectUrl.Port);
		ReportError(NSLOCTEXT("OnlineRacing", "InvalidSessionAddress", "The session advertised an invalid network address."));
		if (!TryDestroySession(ESessionOperation::DestroyingAfterFailedJoin))
		{
			OnJoinSessionComplete.Broadcast(false);
		}
		return;
	}

	UWorld* CurrentWorld = GetWorld();
	APlayerController* LocalPlayerController = nullptr;
	if (IsValid(CurrentWorld))
	{
		LocalPlayerController = CurrentWorld->GetFirstPlayerController();
	}

	if (!IsValid(LocalPlayerController))
	{
		ReportError(NSLOCTEXT("OnlineRacing", "MissingPlayerControllerForSessionTravel", "The local player is unavailable for session travel."));
		if (TryDestroySession(ESessionOperation::DestroyingAfterFailedJoin))
		{
			OnJoinSessionComplete.Broadcast(false);
		}
		return;
	}

	UE_LOG(LogOnlineRacing, Log,
		TEXT("[Local][SessionSubsystem] Session joined. Backend=%s Session=%s Address=%s"),
		*ActiveSubsystemName.ToString(),
		*SessionName.ToString(),
		*ConnectString);
	SessionSearch.Reset();
	OnJoinSessionComplete.Broadcast(true);

	FString TravelUrl = ConnectString;
	if (IsUsingLanBackend())
	{
		TravelUrl += TEXT("?bIsLanMatch");
	}
	LocalPlayerController->ClientTravel(TravelUrl, TRAVEL_Absolute);
}

void UOnlineRacingSessionSubsystem::HandleDestroySessionComplete(const FName SessionName, const bool bWasSuccessful)
{
	const IOnlineSessionPtr PinnedSessionInterface = SessionInterface.Pin();
	if (PinnedSessionInterface.IsValid())
	{
		PinnedSessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
	}
	DestroySessionCompleteDelegateHandle.Reset();

	const bool bRecreateSession = CurrentOperation == ESessionOperation::DestroyingForRecreate;
	const bool bJoinAfterDestroy = CurrentOperation == ESessionOperation::DestroyingForJoin;
	const bool bFailedJoinCleanup = CurrentOperation == ESessionOperation::DestroyingAfterFailedJoin;
	CurrentOperation = ESessionOperation::None;
	FString SuccessState = TEXT("false");
	if (bWasSuccessful)
	{
		SuccessState = TEXT("true");
	}

	FString RecreateState = TEXT("false");
	if (bRecreateSession)
	{
		RecreateState = TEXT("true");
	}

	UE_LOG(LogOnlineRacing, Log,
		TEXT("[Local][SessionSubsystem] Session destroyed. Session=%s Success=%s Recreate=%s"),
		*SessionName.ToString(),
		*SuccessState,
		*RecreateState);

	if (bRecreateSession)
	{
		if (bWasSuccessful)
		{
			HostSession(PendingMaxPublicConnections);
			return;
		}

		ReportError(NSLOCTEXT("OnlineRacing", "ReplaceSessionFailed", "The previous session could not be replaced."));
		OnHostSessionComplete.Broadcast(false);
		return;
	}

	if (bJoinAfterDestroy)
	{
		if (bWasSuccessful)
		{
			JoinSession(PendingJoinResultIndex);
			return;
		}

		ReportError(NSLOCTEXT("OnlineRacing", "ReplaceJoinedSessionFailed", "The previous session connection could not be cleared."));
		OnJoinSessionComplete.Broadcast(false);
		return;
	}

	if (bFailedJoinCleanup)
	{
		if (!bWasSuccessful)
		{
			ReportError(NSLOCTEXT("OnlineRacing", "InvalidSessionCleanupFailed", "The invalid session connection could not be cleared."));
		}
		OnJoinSessionComplete.Broadcast(false);
		return;
	}

	if (!bWasSuccessful)
	{
		ReportError(NSLOCTEXT("OnlineRacing", "DestroySessionFailed", "The session could not be closed."));
	}
	OnDestroySessionComplete.Broadcast(bWasSuccessful);
}

void UOnlineRacingSessionSubsystem::ReportError(const FText& ErrorMessage)
{
	UE_LOG(LogOnlineRacing, Warning, TEXT("[Local][SessionSubsystem] %s"), *ErrorMessage.ToString());
	OnSessionError.Broadcast(ErrorMessage);
}

bool UOnlineRacingSessionSubsystem::CanBeginOperation(const TCHAR* RequestedOperation)
{
	if (CurrentOperation != ESessionOperation::None)
	{
		UE_LOG(LogOnlineRacing, Warning,
			TEXT("[Local][SessionSubsystem] Operation rejected: another request is active. Requested=%s Active=%d"),
			RequestedOperation,
			static_cast<int32>(CurrentOperation));
		ReportError(NSLOCTEXT("OnlineRacing", "SessionOperationInProgress", "Another session operation is already in progress."));
		return false;
	}

	if (!RefreshSessionInterface())
	{
		ReportError(NSLOCTEXT("OnlineRacing", "SessionInterfaceUnavailable", "Online sessions are unavailable."));
		return false;
	}

	return true;
}

bool UOnlineRacingSessionSubsystem::RefreshSessionInterface()
{
	IOnlineSubsystem* OnlineSubsystem = Online::GetSubsystem(GetWorld());
	if (OnlineSubsystem == nullptr)
	{
		SessionInterface.Reset();
		ActiveSubsystemName = NAME_None;
		return false;
	}

	const IOnlineSessionPtr NewSessionInterface = OnlineSubsystem->GetSessionInterface();
	if (!NewSessionInterface.IsValid())
	{
		SessionInterface.Reset();
		ActiveSubsystemName = NAME_None;
		return false;
	}

	SessionInterface = NewSessionInterface;
	ActiveSubsystemName = OnlineSubsystem->GetSubsystemName();

	UE_LOG(LogOnlineRacing, Verbose,
		TEXT("[Local][SessionSubsystem] Session interface refreshed. Subsystem=%s Instance=%s"),
		*OnlineSubsystem->GetSubsystemName().ToString(),
		*OnlineSubsystem->GetInstanceName().ToString());
	return true;
}

bool UOnlineRacingSessionSubsystem::TryDestroySession(const ESessionOperation DestroyOperation)
{
	const IOnlineSessionPtr PinnedSessionInterface = SessionInterface.Pin();
	if (!PinnedSessionInterface.IsValid())
	{
		CurrentOperation = ESessionOperation::None;
		ReportError(NSLOCTEXT("OnlineRacing", "SessionInterfaceExpiredForDestroyOperation", "Online sessions are unavailable."));
		return false;
	}

	CurrentOperation = DestroyOperation;
	DestroySessionCompleteDelegateHandle = PinnedSessionInterface->AddOnDestroySessionCompleteDelegate_Handle(
		FOnDestroySessionCompleteDelegate::CreateUObject(this, &ThisClass::HandleDestroySessionComplete));

	if (PinnedSessionInterface->DestroySession(NAME_GameSession))
	{
		return true;
	}

	PinnedSessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
	DestroySessionCompleteDelegateHandle.Reset();
	CurrentOperation = ESessionOperation::None;
	ReportError(NSLOCTEXT("OnlineRacing", "DestroySessionFailedToStart", "The session could not be closed."));
	return false;
}

void UOnlineRacingSessionSubsystem::ClearOnlineDelegates()
{
	const IOnlineSessionPtr PinnedSessionInterface = SessionInterface.Pin();

	if (CreateSessionCompleteDelegateHandle.IsValid())
	{
		if (PinnedSessionInterface.IsValid())
		{
			PinnedSessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
		}
		CreateSessionCompleteDelegateHandle.Reset();
	}

	if (FindSessionsCompleteDelegateHandle.IsValid())
	{
		if (PinnedSessionInterface.IsValid())
		{
			PinnedSessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
		}
		FindSessionsCompleteDelegateHandle.Reset();
	}

	if (JoinSessionCompleteDelegateHandle.IsValid())
	{
		if (PinnedSessionInterface.IsValid())
		{
			PinnedSessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
		}
		JoinSessionCompleteDelegateHandle.Reset();
	}

	if (DestroySessionCompleteDelegateHandle.IsValid())
	{
		if (PinnedSessionInterface.IsValid())
		{
			PinnedSessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
		}
		DestroySessionCompleteDelegateHandle.Reset();
	}
}



