// Copyright Epic Games, Inc. All Rights Reserved.


#include "TimeTrialTrackGate.h"

#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"

#include "TimeTrialPlayerController.h"

ATimeTrialTrackGate::ATimeTrialTrackGate()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("Collision Box"));
	CollisionBox->SetupAttachment(RootComponent);

	CollisionBox->SetBoxExtent(FVector(1000.0f));
	CollisionBox->SetLineThickness(32.0f);
	CollisionBox->bHiddenInGame = false;
	CollisionBox->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
}

void ATimeTrialTrackGate::NotifyActorBeginOverlap(AActor* OtherActor)
{
	if (!HasAuthority() || !IsValid(OtherActor))
	{
		return;
	}

	ATimeTrialPlayerController* const PlayerController = Cast<ATimeTrialPlayerController>(OtherActor->GetInstigatorController());
	if (!IsValid(PlayerController) || PlayerController->GetTargetGate() != this)
	{
		return;
	}

	PlayerController->SetTargetGate(NextMarker);
	if (bIsFinishLine)
	{
		PlayerController->IncrementLapCount();
	}
}

ATimeTrialTrackGate* ATimeTrialTrackGate::GetNextMarker() const
{
	return NextMarker.Get();
}
