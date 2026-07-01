// Copyright Epic Games, Inc. All Rights Reserved.

#include "HiveWheelRear.h"

UHiveWheelRear::UHiveWheelRear()
{
	AxleType = EAxleType::Rear;
	bAffectedByHandbrake = true;
	bAffectedByEngine = true;
}
