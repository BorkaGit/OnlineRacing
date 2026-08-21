// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "OnlineRacingLobbyGameMode.generated.h"

class AOnlineRacingPlayerState;
class UWorld;

UCLASS(abstract)
class ONLINERACING_API AOnlineRacingLobbyGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	AOnlineRacingLobbyGameMode();

	virtual void Logout(AController* Exiting) override;

	void HandleStartMatchRequest(APlayerController* RequestingPlayer);
	void SetPlayerReady(AOnlineRacingPlayerState* PlayerState, bool bIsReady);

protected:
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lobby", meta = (ClampMin = "1"))
	int32 MinimumPlayersToStart = 2;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Travel", meta = (AllowedClasses = "/Script/Engine.World"))
	TSoftObjectPtr<UWorld> MatchMap;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Travel", meta = (ClampMin = "0.5"))
	float MatchTravelDelay = 1.f;

private:
	void AssignHostIfNeeded(AOnlineRacingPlayerState* JoiningPlayerState, bool bPreferAsHost = false, const AOnlineRacingPlayerState* IgnoredPlayerState = nullptr);
	void EvaluateLobbyState(const AOnlineRacingPlayerState* IgnoredPlayerState = nullptr);
	void TravelToMatch();

	bool bTravelInProgress = false;
	FTimerHandle MatchTravelTimerHandle;
};