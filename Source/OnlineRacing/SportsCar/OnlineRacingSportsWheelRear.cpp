// Copyright Epic Games, Inc. All Rights Reserved.


#include "OnlineRacingSportsWheelRear.h"

UOnlineRacingSportsWheelRear::UOnlineRacingSportsWheelRear()
{
	WheelRadius = 40.f;
	WheelWidth = 40.0f;
	FrictionForceMultiplier = 4.0f;
	SideSlipModifier = 0.6f;
	SlipThreshold = 100.0f;
	SkidThreshold = 100.0f;
	MaxSteerAngle = 0.0f;
	MaxHandBrakeTorque = 8000.0f;
}