// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "OnlineRacingLobbyWidget.generated.h"

class AOnlineRacingLobbyGameState;
class AOnlineRacingPlayerState;
class UButton;
class UTextBlock;

UCLASS()
class ONLINERACING_API UOnlineRacingLobbyWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Lobby")
	TObjectPtr<AOnlineRacingLobbyGameState> OnlineRacingLobbyGameState;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Lobby")
	TObjectPtr<AOnlineRacingPlayerState> OwningOnlineRacingPlayerState;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_PlayerCount;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_ReadyStatus;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_ReadyButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Ready;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Start;

	UFUNCTION()
	void HandleLobbyStateChanged(int32 ReadyPlayerCount, int32 TotalPlayerCount, bool bCanStartMatch, bool bIsTravelingToMatch);

	UFUNCTION()
	void HandleLobbyPlayerStateChanged(bool bIsReady, bool bIsHost);

	UFUNCTION()
	void HandleReadyButtonClicked() const;

	UFUNCTION()
	void HandleStartButtonClicked() const;

	UFUNCTION(BlueprintImplementableEvent, Category = "Lobby", meta = (DisplayName = "Update Lobby State"))
	void BP_UpdateLobbyState(int32 ReadyPlayerCount, int32 TotalPlayerCount, bool bCanStartMatch, bool bIsLocalPlayerReady, bool bIsLocalPlayerHost);

private:
	bool bLocalPlayerReady = false;
};
