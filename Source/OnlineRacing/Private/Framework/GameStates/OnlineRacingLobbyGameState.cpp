// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/GameStates/OnlineRacingLobbyGameState.h"

#include "Net/UnrealNetwork.h"

void AOnlineRacingLobbyGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AOnlineRacingLobbyGameState, LobbyState);
}

void AOnlineRacingLobbyGameState::SetLobbyState(const int32 NewReadyPlayerCount, const int32 NewTotalPlayerCount, const bool bNewCanStartMatch, const bool bNewIsTravelingToMatch)
{
	if (!HasAuthority())
	{
		return;
	}

	const int32 ValidTotalPlayerCount = FMath::Max(0, NewTotalPlayerCount);
	const int32 ValidReadyPlayerCount = FMath::Clamp(NewReadyPlayerCount, 0, ValidTotalPlayerCount);
	if (LobbyState.ReadyPlayerCount == ValidReadyPlayerCount
		&& LobbyState.TotalPlayerCount == ValidTotalPlayerCount
		&& LobbyState.bCanStartMatch == bNewCanStartMatch
		&& LobbyState.bIsTravelingToMatch == bNewIsTravelingToMatch)
	{
		return;
	}

	LobbyState.ReadyPlayerCount = ValidReadyPlayerCount;
	LobbyState.TotalPlayerCount = ValidTotalPlayerCount;
	LobbyState.bCanStartMatch = bNewCanStartMatch;
	LobbyState.bIsTravelingToMatch = bNewIsTravelingToMatch;
	OnRep_LobbyState();
	ForceNetUpdate();
}

void AOnlineRacingLobbyGameState::OnRep_LobbyState()
{
	OnLobbyStateChanged.Broadcast(
		LobbyState.ReadyPlayerCount,
		LobbyState.TotalPlayerCount,
		LobbyState.bCanStartMatch,
		LobbyState.bIsTravelingToMatch);
}