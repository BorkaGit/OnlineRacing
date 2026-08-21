// Copyright Epic Games, Inc. All Rights Reserved.


#include "Vehicle/SportsCar/OnlineRacingSportsWheelRear.h"

UOnlineRacingSportsWheelRear::UOnlineRacingSportsWheelRear()
{
	WheelRadius = 40.f;
	WheelWidth = 40.f;
	FrictionForceMultiplier = 4.f;
	SideSlipModifier = 0.6f;
	SlipThreshold = 100.f;
	SkidThreshold = 100.f;
	MaxSteerAngle = 0.f;
	MaxHandBrakeTorque = 8000.0f;
}