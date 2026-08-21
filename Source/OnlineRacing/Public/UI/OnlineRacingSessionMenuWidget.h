// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Online/OnlineRacingSessionSubsystem.h"
#include "OnlineRacingSessionMenuWidget.generated.h"

class UButton;
class UTextBlock;

UCLASS()
class ONLINERACING_API UOnlineRacingSessionMenuWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Session", meta = (ClampMin = "1"))
	int32 MaxPublicConnections = 4;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Session", meta = (ClampMin = "1"))
	int32 MaxSearchResults = 50;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Host;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Refresh;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Join;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Status;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_SessionResult;

private:
	UFUNCTION()
	void HandleHostButtonClicked();

	UFUNCTION()
	void HandleRefreshButtonClicked();

	UFUNCTION()
	void HandleJoinButtonClicked();

	UFUNCTION()
	void HandleHostSessionComplete(bool bWasSuccessful);

	UFUNCTION()
	void HandleFindSessionsComplete(const TArray<FOnlineRacingSessionInfo>& Sessions, bool bWasSuccessful);

	UFUNCTION()
	void HandleJoinSessionComplete(bool bWasSuccessful);

	UFUNCTION()
	void HandleSessionError(FText ErrorMessage) const;

	void SetOperationInProgress(bool bNewOperationInProgress);
	void SetStatusText(const FText& StatusText) const;
	void UpdateControls() const;

	UPROPERTY(Transient)
	TObjectPtr<UOnlineRacingSessionSubsystem> SessionSubsystem;

	int32 SelectedResultIndex = INDEX_NONE;
	bool bOperationInProgress = false;
};