#include "Race/OnlineRacingRaceCheckpoint.h"

#include "Components/ArrowComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"

#include "OnlineRacingPawn.h"
#include "Race/OnlineRacingRaceGameMode.h"

AOnlineRacingRaceCheckpoint::AOnlineRacingRaceCheckpoint()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;

	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	SetRootComponent(CollisionBox);

	CollisionBox->SetBoxExtent(FVector(100.f, 500.f, 250.f));
	CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionBox->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CollisionBox->SetCollisionResponseToChannel(ECC_Vehicle, ECR_Overlap);
	CollisionBox->SetGenerateOverlapEvents(true);
	CollisionBox->OnComponentBeginOverlap.AddDynamic(this, &AOnlineRacingRaceCheckpoint::HandleCollisionBoxBeginOverlap);

	RespawnPoint = CreateDefaultSubobject<UArrowComponent>(TEXT("RespawnPoint"));
	RespawnPoint->SetupAttachment(CollisionBox);
	RespawnPoint->SetRelativeLocation(FVector(300.f, 0.f, 100.f));
	RespawnPoint->ArrowColor = FColor::Green;
	RespawnPoint->ArrowSize = 2.f;
	RespawnPoint->bIsScreenSizeScaled = true;
}

FTransform AOnlineRacingRaceCheckpoint::GetRespawnTransform() const
{
	return RespawnPoint->GetComponentTransform();
}

void AOnlineRacingRaceCheckpoint::HandleCollisionBoxBeginOverlap(
	UPrimitiveComponent*,
	AActor* OtherActor,
	UPrimitiveComponent*,
	int32,
	bool,
	const FHitResult&)
{
	if (!HasAuthority() || !IsValid(OtherActor))
	{
		return;
	}

	AOnlineRacingPawn* const VehiclePawn = Cast<AOnlineRacingPawn>(OtherActor);
	if (!IsValid(VehiclePawn))
	{
		return;
	}

	AOnlineRacingRaceGameMode* const RaceGameMode = GetWorld()->GetAuthGameMode<AOnlineRacingRaceGameMode>();
	if (!IsValid(RaceGameMode))
	{
		return;
	}

	RaceGameMode->HandleCheckpointReached(*this, *VehiclePawn);
}
