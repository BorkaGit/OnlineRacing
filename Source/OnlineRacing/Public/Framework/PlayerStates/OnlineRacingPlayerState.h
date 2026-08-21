#pragma once

#include "GameFramework/PlayerState.h"
#include "OnlineRacingPlayerState.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnlineRacingPlayerFinishedChanged, bool);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLobbyPlayerStateChanged, bool, bIsReady, bool, bIsHost);


UCLASS()
class ONLINERACING_API AOnlineRacingPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	int32 GetCurrentLap() const { return CurrentLap; }
	int32 GetLastCheckpointIndex() const { return LastCheckpointIndex; }
	int32 GetNextCheckpointIndex() const { return NextCheckpointIndex; }
	int32 GetFinishPosition() const { return FinishPosition; }
	double GetFinishTimeSeconds() const { return FinishTimeSeconds; }
	bool HasFinishedRace() const { return bFinished; }
	FOnlineRacingPlayerFinishedChanged& OnRaceFinishedChanged() { return RaceFinishedChanged; }

	//~ Begin Lobby Settings

	UFUNCTION(BlueprintPure, Category = "Lobby")
	bool IsLobbyReady() const { return bIsLobbyReady; }

	UFUNCTION(BlueprintPure, Category = "Lobby")
	bool IsLobbyHost() const { return bIsLobbyHost; }

	void SetLobbyHost(bool bNewIsHost);
	void SetLobbyReady(bool bNewIsReady);

	UPROPERTY(BlueprintAssignable, Category = "Lobby|Events")
	FOnLobbyPlayerStateChanged OnLobbyPlayerStateChanged;

	//~ End Lobby Settings

protected:
	UPROPERTY(Replicated, VisibleInstanceOnly, BlueprintReadOnly, Category = "Race|Progress")
	int32 CurrentLap = 0;

	UPROPERTY(Replicated, VisibleInstanceOnly, BlueprintReadOnly, Category = "Race|Progress")
	int32 LastCheckpointIndex = INDEX_NONE;

	UPROPERTY(Replicated, VisibleInstanceOnly, BlueprintReadOnly, Category = "Race|Progress")
	int32 NextCheckpointIndex = INDEX_NONE;

	UPROPERTY(ReplicatedUsing = OnRep_Finished, VisibleInstanceOnly, BlueprintReadOnly, Category = "Race|Progress")
	bool bFinished = false;

	UPROPERTY(Replicated, VisibleInstanceOnly, BlueprintReadOnly, Category = "Race|Results")
	int32 FinishPosition = 0;

	UPROPERTY(Replicated, VisibleInstanceOnly, BlueprintReadOnly, Category = "Race|Results", meta = (Units = "s"))
	double FinishTimeSeconds = 0.0;

	UPROPERTY(ReplicatedUsing = OnRep_LobbyState, BlueprintReadOnly, Category = "Lobby")
	bool bIsLobbyReady = false;

	UPROPERTY(ReplicatedUsing = OnRep_LobbyState, BlueprintReadOnly, Category = "Lobby")
	bool bIsLobbyHost = false;

	UFUNCTION()
	void OnRep_LobbyState();

private:
	friend class AOnlineRacingMatchGameMode;

	FOnlineRacingPlayerFinishedChanged RaceFinishedChanged;

	UFUNCTION()
	void OnRep_Finished();

	void FinishRace(int32 NewFinishPosition, double NewFinishTimeSeconds);
	void InitializeRaceProgress(int32 InitialCheckpointIndex);
	void SetRaceProgress(int32 NewCurrentLap, int32 NewLastCheckpointIndex, int32 NewNextCheckpointIndex);
};
