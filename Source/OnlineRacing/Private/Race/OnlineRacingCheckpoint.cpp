#include "Race/OnlineRacingCheckpoint.h"

#include "Components/ArrowComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"

#include "Vehicle/OnlineRacingPawn.h"
#include "Framework/GameModes/OnlineRacingMatchGameMode.h"

AOnlineRacingCheckpoint::AOnlineRacingCheckpoint()
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
	CollisionBox->OnComponentBeginOverlap.AddDynamic(this, &AOnlineRacingCheckpoint::HandleCollisionBoxBeginOverlap);

	RespawnPoint = CreateDefaultSubobject<UArrowComponent>(TEXT("RespawnPoint"));
	RespawnPoint->SetupAttachment(CollisionBox);
	RespawnPoint->SetRelativeLocation(FVector(300.f, 0.f, 100.f));
	RespawnPoint->ArrowColor = FColor::Green;
	RespawnPoint->ArrowSize = 2.f;
	RespawnPoint->bIsScreenSizeScaled = true;
}

FTransform AOnlineRacingCheckpoint::GetRespawnTransform() const
{
	return RespawnPoint->GetComponentTransform();
}

void AOnlineRacingCheckpoint::HandleCollisionBoxBeginOverlap(
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

	AOnlineRacingMatchGameMode* const RaceGameMode = GetWorld()->GetAuthGameMode<AOnlineRacingMatchGameMode>();
	if (!IsValid(RaceGameMode))
	{
		return;
	}

	RaceGameMode->HandleCheckpointReached(*this, *VehiclePawn);
}
