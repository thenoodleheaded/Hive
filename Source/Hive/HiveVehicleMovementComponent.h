// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "HiveVehicleMovementComponent.generated.h"

class UCarPhysicsProfile;

namespace Chaos
{
	class FSimpleWheeledVehicle;
}

UCLASS()
class HIVE_API UHiveVehicleMovementComponent : public UChaosWheeledVehicleMovementComponent
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Profile")
	UCarPhysicsProfile* PhysicsProfile = nullptr;

	virtual void OnCreatePhysicsState() override;
	virtual void SetupVehicle(TUniquePtr<Chaos::FSimpleWheeledVehicle>& PVehicle) override;

private:
	bool bHasPendingProfileValues = false;
	float PendingTorqueMultiplier = 1.0f;
	float PendingMaxSteerAngle = 40.0f;
	float PendingMaxBrakeTorque = 4500.0f;
	float PendingFrictionForceMultiplier = 3.0f;

	UCarPhysicsProfile* ResolvePhysicsProfile() const;
	void PrepareProfileValues();
};
