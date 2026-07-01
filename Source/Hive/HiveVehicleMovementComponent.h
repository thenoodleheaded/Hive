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

UENUM(BlueprintType)
enum class EHiveDriftState : uint8
{
	Gripping,
	Initiating,
	Drifting,
	Recovering
};

UCLASS()
class HIVE_API UHiveVehicleMovementComponent : public UChaosWheeledVehicleMovementComponent
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Profile")
	UCarPhysicsProfile* PhysicsProfile = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Drift")
	EHiveDriftState CurrentDriftState = EHiveDriftState::Gripping;

	UPROPERTY(EditAnywhere, Category = "Drift")
	float SteeringDriftThreshold = 0.6f;

	UPROPERTY(EditAnywhere, Category = "Drift")
	float ThrottleDriftThreshold = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Drift")
	float LateralVelocityDriftThreshold = 300.0f;

	UPROPERTY(EditAnywhere, Category = "Drift")
	float LateralVelocityRecoveryThreshold = 180.0f;

	UPROPERTY(EditAnywhere, Category = "Drift")
	float DriftSpeedRetentionMin = 0.72f;

	UPROPERTY(EditAnywhere, Category = "Drift")
	float DriftSpeedRetentionMax = 0.80f;

	UPROPERTY(EditAnywhere, Category = "Drift")
	float SpeedRetentionLerpSpeed = 3.0f;

	UPROPERTY(EditAnywhere, Category = "Drift")
	float LateralVelocitySustainFactor = 0.95f;

	virtual void OnCreatePhysicsState() override;
	virtual void SetupVehicle(TUniquePtr<Chaos::FSimpleWheeledVehicle>& PVehicle) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	int32 DriftStateTickCount = 0;
	float DriftEntrySpeed = 0.0f;
	float DriftTargetRetainedSpeed = 0.0f;
	float DriftActualForwardSpeed = 0.0f;
	bool bHasPendingProfileValues = false;
	float PendingMaxSteerAngle = 40.0f;
	float PendingMaxBrakeTorque = 4500.0f;
	float PendingFrictionForceMultiplier = 3.0f;

	UCarPhysicsProfile* ResolvePhysicsProfile() const;
	void PrepareProfileValues();
	float GetSignedLateralVelocity() const;
	void UpdateDriftState(float LateralVelocity, float SteeringInput, float ThrottleInput, bool bHandbrakeActive);
	void SetDriftState(EHiveDriftState NewState, float LateralVelocity, float SteeringInput, float ThrottleInput);
	void ApplyDriftVelocityManipulation(float DeltaTime);
};
