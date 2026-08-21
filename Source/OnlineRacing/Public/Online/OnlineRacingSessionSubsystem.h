// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "UObject/SoftObjectPath.h"
#include "OnlineRacingSessionSubsystem.generated.h"

class FOnlineSessionSearch;

USTRUCT(BlueprintType)
struct FOnlineRacingSessionInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Session")
	int32 ResultIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "Session")
	FString OwningPlayerName;

	UPROPERTY(BlueprintReadOnly, Category = "Session")
	int32 CurrentPlayers = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Session")
	int32 MaxPlayers = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Session")
	int32 PingInMs = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnOnlineRacingHostSessionComplete, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnOnlineRacingFindSessionsComplete, const TArray<FOnlineRacingSessionInfo>&, Sessions, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnOnlineRacingJoinSessionComplete, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnOnlineRacingDestroySessionComplete, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnOnlineRacingSessionError, FText, ErrorMessage);

UCLASS(Config = Game, DefaultConfig)
class ONLINERACING_API UOnlineRacingSessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UOnlineRacingSessionSubsystem();

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "OnlineRacing|Session")
	void HostSession(int32 MaxPublicConnections = 4);

	UFUNCTION(BlueprintCallable, Category = "OnlineRacing|Session")
	void FindSessions(int32 MaxSearchResults = 50);

	UFUNCTION(BlueprintCallable, Category = "OnlineRacing|Session")
	void JoinSession(int32 ResultIndex);

	UFUNCTION(BlueprintCallable, Category = "OnlineRacing|Session")
	void DestroySession();

	UFUNCTION(BlueprintPure, Category = "OnlineRacing|Session")
	bool HasActiveSession() const;

	UFUNCTION(BlueprintPure, Category = "OnlineRacing|Session")
	FName GetActiveSubsystemName() const;

	UFUNCTION(BlueprintPure, Category = "OnlineRacing|Session")
	bool IsUsingLanBackend() const;

	UPROPERTY(BlueprintAssignable, Category = "OnlineRacing|Session|Events")
	FOnOnlineRacingHostSessionComplete OnHostSessionComplete;

	UPROPERTY(BlueprintAssignable, Category = "OnlineRacing|Session|Events")
	FOnOnlineRacingFindSessionsComplete OnFindSessionsComplete;

	UPROPERTY(BlueprintAssignable, Category = "OnlineRacing|Session|Events")
	FOnOnlineRacingJoinSessionComplete OnJoinSessionComplete;

	UPROPERTY(BlueprintAssignable, Category = "OnlineRacing|Session|Events")
	FOnOnlineRacingDestroySessionComplete OnDestroySessionComplete;

	UPROPERTY(BlueprintAssignable, Category = "OnlineRacing|Session|Events")
	FOnOnlineRacingSessionError OnSessionError;

private:
	enum class ESessionOperation : uint8
	{
		None,
		Creating,
		Finding,
		Joining,
		Destroying,
		DestroyingForRecreate,
		DestroyingForJoin,
		DestroyingAfterFailedJoin
	};

	void HandleCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void HandleFindSessionsComplete(bool bWasSuccessful);
	void HandleJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void HandleDestroySessionComplete(FName SessionName, bool bWasSuccessful);
	void ReportError(const FText& ErrorMessage);
	bool CanBeginOperation(const TCHAR* RequestedOperation);
	bool RefreshSessionInterface();
	bool TryDestroySession(ESessionOperation DestroyOperation);
	void ClearOnlineDelegates();

	UPROPERTY(Config)
	FSoftObjectPath LobbyMapPath;

	TWeakPtr<IOnlineSession, ESPMode::ThreadSafe> SessionInterface;
	TSharedPtr<FOnlineSessionSearch> SessionSearch;
	FName ActiveSubsystemName = NAME_None;

	FDelegateHandle CreateSessionCompleteDelegateHandle;
	FDelegateHandle FindSessionsCompleteDelegateHandle;
	FDelegateHandle JoinSessionCompleteDelegateHandle;
	FDelegateHandle DestroySessionCompleteDelegateHandle;

	ESessionOperation CurrentOperation = ESessionOperation::None;
	int32 PendingMaxPublicConnections = 4;
	int32 PendingJoinResultIndex = INDEX_NONE;
};