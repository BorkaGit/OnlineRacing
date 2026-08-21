// Copyright Epic Games, Inc. All Rights Reserved.

#include "Vehicle/Wheels/OnlineRacingWheelFront.h"
#include "UObject/ConstructorHelpers.h"

UOnlineRacingWheelFront::UOnlineRacingWheelFront()
{
	AxleType = EAxleType::Front;
	bAffectedBySteering = true;
	MaxSteerAngle = 40.f;
}