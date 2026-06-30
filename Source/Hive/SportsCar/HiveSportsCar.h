// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HivePawn.h"
#include "HiveSportsCar.generated.h"

class UCarPhysicsProfile;
class UHiveVehicleMovementComponent;

/**
 *  Sports car wheeled vehicle implementation
 */
UCLASS(abstract)
class AHiveSportsCar : public AHivePawn
{
	GENERATED_BODY()

protected:

	UPROPERTY(EditAnywhere, Category = "Profile")
	UCarPhysicsProfile* PhysicsProfile = nullptr;
		
public:

	AHiveSportsCar(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UCarPhysicsProfile* GetPhysicsProfile() const { return PhysicsProfile; }

private:

	void SyncPhysicsProfileToMovementComponent() const;
};
