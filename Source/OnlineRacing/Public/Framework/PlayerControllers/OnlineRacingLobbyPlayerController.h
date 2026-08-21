// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "OnlineRacingLobbyPlayerController.generated.h"

class UOnlineRacingLobbyWidget;

UCLASS(abstract)
class ONLINERACING_API AOnlineRacingLobbyPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Lobby")
	void SetLobbyReady(bool bIsReady);

	UFUNCTION(BlueprintCallable, Category = "Lobby")
	void RequestLobbyMatchStart();

protected:
	virtual void BeginPlay() override;
	virtual void OnRep_PlayerState() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Lobby")
	TSubclassOf<UOnlineRacingLobbyWidget> LobbyWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UOnlineRacingLobbyWidget> LobbyWidget;

	UFUNCTION(Server, Reliable)
	void Server_SetLobbyReady(bool bIsReady);

	UFUNCTION(Server, Reliable)
	void Server_RequestLobbyMatchStart();

private:
	void ConfigureLobbyInputMode();
	void RemoveLobbyWidget();
	void TryCreateLobbyWidget();


};
