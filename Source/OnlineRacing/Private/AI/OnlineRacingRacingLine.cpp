// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/OnlineRacingRacingLine.h"

#include "Components/SplineComponent.h"

AOnlineRacingRacingLine::AOnlineRacingRacingLine()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;
	
	SplineComponent = CreateDefaultSubobject<USplineComponent>(TEXT("SplineComponent"));
	
	SetRootComponent(SplineComponent);
	SplineComponent->SetClosedLoop(true);
}