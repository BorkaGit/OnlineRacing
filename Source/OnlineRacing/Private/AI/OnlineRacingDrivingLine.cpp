// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/OnlineRacingDrivingLine.h"

#include "Components/SplineComponent.h"

AOnlineRacingDrivingLine::AOnlineRacingDrivingLine()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;

	SplineComponent = CreateDefaultSubobject<USplineComponent>(TEXT("SplineComponent"));

	SetRootComponent(SplineComponent);
	SplineComponent->SetClosedLoop(true);
}
