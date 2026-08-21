// Copyright Epic Games, Inc. All Rights Reserved.


#include "Vehicle/SportsCar/OnlineRacingSportsWheelFront.h"

UOnlineRacingSportsWheelFront::UOnlineRacingSportsWheelFront()
{
	WheelRadius = 39.f;
	WheelWidth = 35.f;
	FrictionForceMultiplier = 3.f;

	MaxBrakeTorque = 4500.f;
	MaxHandBrakeTorque = 6000.f;
}