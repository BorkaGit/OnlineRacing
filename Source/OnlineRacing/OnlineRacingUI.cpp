// Copyright Epic Games, Inc. All Rights Reserved.


#include "OnlineRacingUI.h"

void UOnlineRacingUI::UpdateSpeed(float NewSpeed)
{
	float SpeedConversion = 0.036f;
	if (bIsMPH)
	{
		SpeedConversion = 0.022f;
	}

	const float FormattedSpeed = FMath::Abs(NewSpeed) * SpeedConversion;
	OnSpeedUpdate(FormattedSpeed);
}

void UOnlineRacingUI::UpdateGear(int32 NewGear)
{
	OnGearUpdate(NewGear);
}
