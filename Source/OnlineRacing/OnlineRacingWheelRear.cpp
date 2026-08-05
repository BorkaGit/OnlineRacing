// Copyright Epic Games, Inc. All Rights Reserved.

#include "OnlineRacingWheelRear.h"
#include "UObject/ConstructorHelpers.h"

UOnlineRacingWheelRear::UOnlineRacingWheelRear()
{
	AxleType = EAxleType::Rear;
	bAffectedByHandbrake = true;
	bAffectedByEngine = true;
}