// Copyright Epic Games, Inc. All Rights Reserved.


#include "HiveUI.h"

void UHiveUI::UpdateSpeed(float NewSpeed)
{
	// format the speed to KPH or MPH
	float FormattedSpeed = FMath::Abs(NewSpeed) * (bIsMPH ? 0.022f : 0.036f);

	// call the Blueprint handler
	OnSpeedUpdate(FormattedSpeed);
}

void UHiveUI::UpdateGear(int32 NewGear)
{
	// call the Blueprint handler
	OnGearUpdate(NewGear);
}