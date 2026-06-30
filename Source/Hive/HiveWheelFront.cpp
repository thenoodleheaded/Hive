// Copyright Epic Games, Inc. All Rights Reserved.

#include "HiveWheelFront.h"
#include "UObject/ConstructorHelpers.h"

UHiveWheelFront::UHiveWheelFront()
{
	AxleType = EAxleType::Front;
	bAffectedBySteering = true;
	MaxSteerAngle = 40.f;
}