// Copyright Epic Games, Inc. All Rights Reserved.

#include "HiveVehicleMovementComponent.h"
#include "HiveSportsCar.h"
#include "UCarPhysicsProfile.h"

namespace
{
	float MapProfileStat(float NormalisedStat, float MinValue, float MaxValue)
	{
		return FMath::Lerp(MinValue, MaxValue, FMath::Clamp(NormalisedStat, 0.0f, 1.0f));
	}
}

UCarPhysicsProfile* UHiveVehicleMovementComponent::ResolvePhysicsProfile() const
{
	if (IsValid(PhysicsProfile))
	{
		return PhysicsProfile;
	}

	if (const AHiveSportsCar* SportsCar = Cast<AHiveSportsCar>(GetOwner()))
	{
		return SportsCar->GetPhysicsProfile();
	}

	return nullptr;
}

void UHiveVehicleMovementComponent::PrepareProfileValues()
{
	PhysicsProfile = ResolvePhysicsProfile();
	bHasPendingProfileValues = false;

	if (!IsValid(PhysicsProfile))
	{
		return;
	}

	const float SpeedStat = PhysicsProfile->GetNormalisedStat(GET_MEMBER_NAME_CHECKED(UCarPhysicsProfile, Speed));
	const float AccelerationStat = PhysicsProfile->GetNormalisedStat(GET_MEMBER_NAME_CHECKED(UCarPhysicsProfile, Acceleration));
	const float HandlingStat = PhysicsProfile->GetNormalisedStat(GET_MEMBER_NAME_CHECKED(UCarPhysicsProfile, Handling));
	const float BrakingStat = PhysicsProfile->GetNormalisedStat(GET_MEMBER_NAME_CHECKED(UCarPhysicsProfile, Braking));
	const float GripStat = PhysicsProfile->GetNormalisedStat(GET_MEMBER_NAME_CHECKED(UCarPhysicsProfile, Grip));

	EngineSetup.MaxRPM = MapProfileStat(SpeedStat, 5000.0f, 9000.0f);

	PendingTorqueMultiplier = MapProfileStat(AccelerationStat, 0.7f, 1.4f);
	EngineSetup.MaxTorque = 750.0f * PendingTorqueMultiplier;

	PendingMaxSteerAngle = MapProfileStat(HandlingStat, 25.0f, 45.0f);
	PendingMaxBrakeTorque = MapProfileStat(BrakingStat, 3000.0f, 6000.0f);
	PendingFrictionForceMultiplier = MapProfileStat(GripStat, 2.0f, 5.0f);
	bHasPendingProfileValues = true;
}

void UHiveVehicleMovementComponent::OnCreatePhysicsState()
{
	PrepareProfileValues();

	Super::OnCreatePhysicsState();
}

void UHiveVehicleMovementComponent::SetupVehicle(TUniquePtr<Chaos::FSimpleWheeledVehicle>& PVehicle)
{
	Super::SetupVehicle(PVehicle);

	if (!bHasPendingProfileValues || !PVehicle)
	{
		return;
	}

	const int32 ProfiledWheelCount = FMath::Min(4, PVehicle->Wheels.Num());
	for (int32 WheelIndex = 0; WheelIndex < ProfiledWheelCount; ++WheelIndex)
	{
		Chaos::FSimpleWheelSim& VehicleWheel = PVehicle->Wheels[WheelIndex];
		VehicleWheel.MaxBrakeTorque = PendingMaxBrakeTorque;
		VehicleWheel.FrictionMultiplier = PendingFrictionForceMultiplier;

		if (WheelIndex < 2)
		{
			VehicleWheel.MaxSteeringAngle = PendingMaxSteerAngle;
		}
	}
}
