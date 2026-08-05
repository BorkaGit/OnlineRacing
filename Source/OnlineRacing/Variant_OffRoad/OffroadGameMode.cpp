// Copyright Epic Games, Inc. All Rights Reserved.


#include "Variant_OffRoad/OffroadGameMode.h"

#include "Engine/World.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"

void AOffroadGameMode::BeginPlay()
{
	Super::BeginPlay();

	// Player zero is created by the normal game initialization path.
	for (int32 PlayerIndex = 2; PlayerIndex <= NumberOfLocalPlayers; ++PlayerIndex)
	{
		UGameplayStatics::CreatePlayer(GetWorld(), -1, true);
	}
}

AActor* AOffroadGameMode::ChoosePlayerStart_Implementation(AController*)
{
	const FName PlayerTag(*FString::Printf(TEXT("Player%d"), CurrentPlayerStartAssignment));
	TArray<AActor*> PlayerStarts;
	UGameplayStatics::GetAllActorsOfClassWithTag(GetWorld(), APlayerStart::StaticClass(), PlayerTag, PlayerStarts);

	++CurrentPlayerStartAssignment;
	if (PlayerStarts.IsEmpty())
	{
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), PlayerStarts);
	}

	if (PlayerStarts.IsEmpty())
	{
		return nullptr;
	}

	return PlayerStarts[FMath::RandRange(0, PlayerStarts.Num() - 1)];
}
