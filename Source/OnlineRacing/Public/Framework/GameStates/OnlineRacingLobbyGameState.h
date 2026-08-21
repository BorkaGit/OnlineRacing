// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "OnlineRacingLobbyGameState.generated.h"

USTRUCT(BlueprintType)
struct FOnlineRacingLobbyState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Lobby")
	int32 ReadyPlayerCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Lobby")
	int32 TotalPlayerCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Lobby")
	bool bCanStartMatch = false;

	UPROPERTY(BlueprintReadOnly, Category = "Lobby")
	bool bIsTravelingToMatch = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnOnlineRacingLobbyStateChanged, int32, ReadyPlayerCount, int32, TotalPlayerCount, bool, bCanStartMatch, bool, bIsTravelingToMatch);

UCLASS()
class ONLINERACING_API AOnlineRacingLobbyGameState : public AGameState
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category = "Lobby")
	FORCEINLINE int32 GetReadyPlayerCount() const { return LobbyState.ReadyPlayerCount; }

	UFUNCTION(BlueprintPure, Category = "Lobby")
	FORCEINLINE int32 GetTotalPlayerCount() const { return LobbyState.TotalPlayerCount; }

	UFUNCTION(BlueprintPure, Category = "Lobby")
	FORCEINLINE bool CanStartMatch() const { return LobbyState.bCanStartMatch; }

	UFUNCTION(BlueprintPure, Category = "Lobby")
	FORCEINLINE bool IsTravelingToMatch() const { return LobbyState.bIsTravelingToMatch; }

	UPROPERTY(BlueprintAssignable, Category = "Lobby|Events")
	FOnOnlineRacingLobbyStateChanged OnLobbyStateChanged;

	void SetLobbyState(int32 NewReadyPlayerCount, int32 NewTotalPlayerCount, bool bNewCanStartMatch, bool bNewIsTravelingToMatch);

protected:
	UPROPERTY(ReplicatedUsing = OnRep_LobbyState, BlueprintReadOnly, Category = "Lobby")
	FOnlineRacingLobbyState LobbyState;

	UFUNCTION()
	void OnRep_LobbyState();
};