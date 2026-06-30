// Copyright Epic Games, Inc. All Rights Reserved.

#include "UCarPhysicsProfile.h"
#include "Hive.h"

float UCarPhysicsProfile::GetNormalisedStat(FName StatName) const
{
	float StatValue = 50.0f;

	if (StatName == GET_MEMBER_NAME_CHECKED(UCarPhysicsProfile, Speed))
	{
		StatValue = Speed;
	}
	else if (StatName == GET_MEMBER_NAME_CHECKED(UCarPhysicsProfile, Acceleration))
	{
		StatValue = Acceleration;
	}
	else if (StatName == GET_MEMBER_NAME_CHECKED(UCarPhysicsProfile, Handling))
	{
		StatValue = Handling;
	}
	else if (StatName == GET_MEMBER_NAME_CHECKED(UCarPhysicsProfile, Braking))
	{
		StatValue = Braking;
	}
	else if (StatName == GET_MEMBER_NAME_CHECKED(UCarPhysicsProfile, Grip))
	{
		StatValue = Grip;
	}
	else if (StatName == GET_MEMBER_NAME_CHECKED(UCarPhysicsProfile, Drift))
	{
		StatValue = Drift;
	}
	else if (StatName == GET_MEMBER_NAME_CHECKED(UCarPhysicsProfile, Stability))
	{
		StatValue = Stability;
	}
	else if (StatName == GET_MEMBER_NAME_CHECKED(UCarPhysicsProfile, Armor))
	{
		StatValue = Armor;
	}
	else if (StatName == GET_MEMBER_NAME_CHECKED(UCarPhysicsProfile, Impact))
	{
		StatValue = Impact;
	}
	else if (StatName == GET_MEMBER_NAME_CHECKED(UCarPhysicsProfile, Nitro))
	{
		StatValue = Nitro;
	}
	else
	{
		UE_LOG(LogHive, Warning, TEXT("Unknown car physics stat '%s'. Returning neutral value."), *StatName.ToString());
		return 0.5f;
	}

	return FMath::Clamp(StatValue / 100.0f, 0.0f, 1.0f);
}
