// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "OnlineRacingPlayerController.generated.h"

class AOnlineRacingPawn;
class AOnlineRacingMatchGameState;
class AOnlineRacingMatchPlayerState;
class UInputMappingContext;
class UOnlineRacingDebugWidget;
class UOnlineRacingCountdownWidget;
class UOnlineRacingResultsWidget;
class UOnlineRacingUI;
class UUserWidget;
enum class EOnlineRacingMatchPhase : uint8;
struct FOnlineRacingMatchResult;

UCLASS(Abstract, Config = "Game")
class AOnlineRacingPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Input|Input Mappings")
	TArray<TObjectPtr<UInputMappingContext>> DefaultMappingContexts;

	UPROPERTY(EditDefaultsOnly, Category = "Input|Input Mappings")
	TArray<TObjectPtr<UInputMappingContext>> MobileExcludedMappingContexts;

	UPROPERTY(EditDefaultsOnly, Category = "Input|Touch Controls")
	TSubclassOf<UUserWidget> MobileControlsWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> MobileControlsWidget;

	UPROPERTY(EditDefaultsOnly, Config, Category = "Input|Touch Controls")
	bool bForceTouchControls = false;

	UPROPERTY(EditDefaultsOnly, Category = "Input|Steering Wheel Controls")
	bool bUseSteeringWheelControls = false;

	UPROPERTY(EditDefaultsOnly, Category = "Input|Steering Wheel Controls", meta = (EditCondition = "bUseSteeringWheelControls"))
	TObjectPtr<UInputMappingContext> SteeringWheelInputMappingContext;

	UPROPERTY(Transient)
	TObjectPtr<AOnlineRacingPawn> VehiclePawn;

	UPROPERTY(EditDefaultsOnly, Category = "Vehicle|UI")
	TSubclassOf<UOnlineRacingUI> VehicleUIClass;

	UPROPERTY(Transient)
	TObjectPtr<UOnlineRacingUI> VehicleUI;

	UPROPERTY(EditDefaultsOnly, Category = "Vehicle|UI")
	TSubclassOf<UOnlineRacingDebugWidget> DebugWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UOnlineRacingDebugWidget> DebugWidget;

	UPROPERTY(EditDefaultsOnly, Category = "Race|UI")
	TSubclassOf<UOnlineRacingCountdownWidget> RaceCountdownWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UOnlineRacingCountdownWidget> RaceCountdownWidget;

	UPROPERTY(EditDefaultsOnly, Category = "Race|UI")
	TSubclassOf<UOnlineRacingResultsWidget> RaceResultsWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UOnlineRacingResultsWidget> RaceResultsWidget;

	UPROPERTY(EditDefaultsOnly, Category = "Race|UI", meta = (ClampMin = "0.0", Units = "s"))
	float RaceStartedDisplayDuration = 1.f;

public:
	virtual void Tick(float DeltaSeconds) override;
	void RequestVehicleRespawn();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnRep_Pawn() override;
	virtual void OnRep_PlayerState() override;
	virtual void SetupInputComponent() override;

private:
	UFUNCTION()
	void OnPawnDestroyed(AActor* DestroyedPawn);

	UFUNCTION(Server, Reliable)
	void Server_RequestVehicleRespawn();

	void ApplyRaceInputStateToVehicle();
	void BindRaceGameState();
	void BindRacePlayerState();
	void CacheVehiclePawn();
	void HandlePlayerFinishedChanged(bool bFinished);
	void HandleRacePhaseChanged(EOnlineRacingMatchPhase NewRacePhase);
	void HandleRaceResultsChanged(const TArray<FOnlineRacingMatchResult>& RaceResults);
	void HandleVehicleRespawnRequest();
	void HideCountdownWidget();
	void HideRaceResults();
	void PresentRaceStart();
	void SetDebugWidgetVisible(bool bVisible);
	void ShowRaceResults();
	void SetVehicleRaceInputEnabled(bool bEnabled);
	bool ShouldUseTouchControls() const;
	void UnbindRaceGameState();
	void UnbindRacePlayerState();
	void UpdateRaceCountdown();

	TWeakObjectPtr<AOnlineRacingMatchGameState> RaceGameState;
	TWeakObjectPtr<AOnlineRacingMatchPlayerState> RacePlayerState;
	FDelegateHandle RacePhaseChangedHandle;
	FDelegateHandle RaceFinishedChangedHandle;
	FDelegateHandle RaceResultsChangedHandle;
	FTimerHandle RaceStartedDisplayTimer;
	bool bRaceStartPresented = false;
};
