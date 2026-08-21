// Copyright Epic Games, Inc. All Rights Reserved.

#include "Vehicle/Wheels/OnlineRacingWheelRear.h"
#include "UObject/ConstructorHelpers.h"

UOnlineRacingWheelRear::UOnlineRacingWheelRear()
{
	AxleType = EAxleType::Rear;
	bAffectedByHandbrake = true;
	bAffectedByEngine = true;
}