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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Drift|Feel", meta = (ToolTip = "Current drift state for debugging and Blueprint reads."))
	EHiveDriftState CurrentDriftState = EHiveDriftState::Gripping;

	UPROPERTY(EditAnywhere, Category = "Drift|Entry", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", ToolTip = "How hard you must brake to trigger drift entry."))
	float BrakeDriftThreshold = 0.3f;

	UPROPERTY(EditAnywhere, Category = "Drift|Entry", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", ToolTip = "Minimum steering for handbrake tap drift entry."))
	float SteerDriftThreshold = 0.35f;

	UPROPERTY(EditAnywhere, Category = "Drift|Entry", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", ToolTip = "Minimum steering for brake drift entry."))
	float MinSteerForBrakeDrift = 0.55f;

	UPROPERTY(EditAnywhere, Category = "Drift|Entry", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", ToolTip = "How long entry input must be held."))
	float InitiatingHoldTime = 0.15f;

	UPROPERTY(EditAnywhere, Category = "Drift|Recovery", meta = (ClampMin = "0.1", ClampMax = "2.0", UIMin = "0.1", UIMax = "2.0", ToolTip = "Time allowed for slip to build before recovery."))
	float DriftNoSlipTimeout = 0.6f;

	UPROPERTY(EditAnywhere, Category = "Drift|Recovery", meta = (ClampMin = "1.0", ClampMax = "20.0", UIMin = "1.0", UIMax = "20.0", ToolTip = "Slip angle needed to finish recovery."))
	float SlipAngleRecoveryThreshold = 8.0f;

	UPROPERTY(EditAnywhere, Category = "Drift|Recovery", meta = (ClampMin = "5.0", ClampMax = "45.0", UIMin = "5.0", UIMax = "45.0", ToolTip = "How straight the car must get before exiting drift."))
	float SlipAngleDriftExitThreshold = 22.0f;

	UPROPERTY(EditAnywhere, Category = "Drift|Feel", meta = (ClampMin = "10.0", ClampMax = "60.0", UIMin = "10.0", UIMax = "60.0", ToolTip = "Slip angle treated as a full drift."))
	float SlipAngleFullDriftDegrees = 35.0f;

	UPROPERTY(EditAnywhere, Category = "Drift|Feel", meta = (ToolTip = "Forces throttle during drift to maintain power."))
	bool bOverrideThrottleDuringDrift = true;

	UPROPERTY(EditAnywhere, Category = "Drift|Grip", meta = (ClampMin = "0.1", ClampMax = "1.0", UIMin = "0.1", UIMax = "1.0", ToolTip = "Overall rear grip during drift. Lower = looser."))
	float DriftGripMultiplier = 0.75f;

	UPROPERTY(EditAnywhere, Category = "Drift|Grip", meta = (ClampMin = "0.5", ClampMax = "10.0", UIMin = "0.5", UIMax = "10.0", ToolTip = "How fast grip changes when entering or leaving drift."))
	float GripBlendSpeed = 2.5f;

	UPROPERTY(EditAnywhere, Category = "Drift|Grip", meta = (ClampMin = "0.5", ClampMax = "6.0", UIMin = "0.5", UIMax = "6.0", ToolTip = "Rear grip during normal driving. Higher = more stable."))
	float GrippingRearFriction = 3.4f;

	UPROPERTY(EditAnywhere, Category = "Drift|Grip", meta = (ClampMin = "0.5", ClampMax = "6.0", UIMin = "0.5", UIMax = "6.0", ToolTip = "Rear grip during drift. Higher = more control, less spin."))
	float DriftingRearFriction = 2.6f;

	virtual void OnCreatePhysicsState() override;
	virtual void SetupVehicle(TUniquePtr<Chaos::FSimpleWheeledVehicle>& PVehicle) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	float GetCurrentSlipAngleDegrees() const { return CurrentSlipAngleDegrees; }

private:
	float InitiatingTimer = 0.0f;
	float DriftingTimer = 0.0f;
	float CurrentSlipAngleDegrees = 0.0f;
	float CurrentGripBlendFactor = 1.0f;
	float LastLoggedGripBlendFactor = 1.0f;
	float CurrentAppliedFrictionMultiplier = 0.0f;
	float CurrentRearFriction = 3.4f;
	float CurrentAppliedRearFriction = 3.4f;
	TArray<float> BaseWheelFrictionMultipliers;
	bool bLastHandbrakeState = false;
	bool bInitiatedByHandbrakeTap = false;
	bool bDriftAngleHasBuilt = false;
	bool bRecoveryAngleConfirmed = false;
	bool bHasPendingProfileValues = false;
	float PendingMaxSteerAngle = 40.0f;
	float PendingMaxBrakeTorque = 4500.0f;
	float PendingFrictionForceMultiplier = 3.0f;

	UCarPhysicsProfile* ResolvePhysicsProfile() const;
	void PrepareProfileValues();
	float ComputeBodySlipAngleDegrees() const;
	float GetBrakeInput();
	void UpdateDriftState(float DeltaTime, float BrakeInput, float SteeringInput, bool bHandbrakeActive);
	void SetDriftState(EHiveDriftState NewState);
	void UpdateGripBlend(float DeltaTime);
	void ApplyGripBlendToWheels();
	void LogGripBlendIfNeeded(float TargetGripBlendFactor);
	void UpdateRearFriction(float DeltaTime);
	float GetTargetRearFriction() const;
};
