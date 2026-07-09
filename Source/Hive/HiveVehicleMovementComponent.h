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

	UPROPERTY(EditAnywhere, Category = "Drift|Entry", meta = (ClampMin = "0.0", ClampMax = "2000.0", UIMin = "0.0", UIMax = "1000.0", ToolTip = "Minimum forward speed required to enter drift."))
	float MinForwardSpeedForDrift = 200.0f;

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

	UPROPERTY(EditAnywhere, Category = "Drift|Feel", meta = (ToolTip = "Enables physics yaw damping during drift."))
	bool bEnableYawDamping = true;

	UPROPERTY(EditAnywhere, Category = "Drift|Feel", meta = (ClampMin = "0.0", ClampMax = "20.0", UIMin = "0.0", UIMax = "20.0", ToolTip = "Physics torque opposing yaw spin during drift."))
	float YawDampingStrength = 2.0f;

	UPROPERTY(EditAnywhere, Category = "Drift|Feel", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", ToolTip = "Temporary scale for testing yaw damping safely."))
	float YawDampingDebugScale = 0.01f;

	UPROPERTY(EditAnywhere, Category = "Drift|Grip", meta = (ClampMin = "0.1", ClampMax = "1.0", UIMin = "0.1", UIMax = "1.0", ToolTip = "Overall rear grip during drift. Lower = looser."))
	float DriftGripMultiplier = 0.75f;

	UPROPERTY(EditAnywhere, Category = "Drift|Grip", meta = (ClampMin = "0.5", ClampMax = "10.0", UIMin = "0.5", UIMax = "10.0", ToolTip = "How fast grip changes when entering or leaving drift."))
	float GripBlendSpeed = 2.5f;

	UPROPERTY(EditAnywhere, Category = "Drift|Grip", meta = (ClampMin = "0.5", ClampMax = "6.0", UIMin = "0.5", UIMax = "6.0", ToolTip = "Rear grip during normal driving. Higher = more stable."))
	float GrippingRearFriction = 3.4f;

	UPROPERTY(EditAnywhere, Category = "Drift|Grip", meta = (ClampMin = "0.5", ClampMax = "6.0", UIMin = "0.5", UIMax = "6.0", ToolTip = "Rear grip during drift. Higher = more control, less spin."))
	float DriftingRearFriction = 2.6f;

	UPROPERTY(EditAnywhere, Category = "Steering|Speed", meta = (ClampMin = "0.1", ClampMax = "1.0", UIMin = "0.1", UIMax = "1.0", ToolTip = "Low-speed steering angle scale. Lower = calmer slow turns."))
	float MinSteeringSpeedScale = 0.55f;

	UPROPERTY(EditAnywhere, Category = "Steering|Speed", meta = (ClampMin = "0.1", ClampMax = "1.5", UIMin = "0.1", UIMax = "1.5", ToolTip = "High-speed steering angle scale."))
	float MaxSteeringSpeedScale = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Steering|Speed", meta = (ClampMin = "100.0", ClampMax = "8000.0", UIMin = "500.0", UIMax = "8000.0", ToolTip = "Speed in cm/s where steering reaches full scale."))
	float SteeringTopSpeedReference = 3000.0f;

	virtual void OnCreatePhysicsState() override;
	virtual void SetupVehicle(TUniquePtr<Chaos::FSimpleWheeledVehicle>& PVehicle) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	float GetCurrentSlipAngleDegrees() const { return CurrentSlipAngleDegrees; }
	bool IsInputFrozen() const { return bInputFrozen; }
	float GetFrozenThrottleValue() const { return FrozenThrottleValue; }
	float GetFrozenBrakeValue() const { return FrozenBrakeValue; }
	float GetFrozenSteeringValue() const { return FrozenSteeringValue; }
	float GetTemporarySteeringMultiplier() const { return TemporarySteeringMultiplier; }
	bool IsTemporarySteeringMultiplierActive() const { return bTemporarySteeringMultiplierActive; }
	bool IsSpeedCapActive() const { return bSpeedCapActive; }
	float GetActiveSpeedCap() const { return ActiveSpeedCap; }
	void ApplyFullInputFreeze(float Duration);
	void ApplyTemporarySteeringMultiplier(float Multiplier, float Duration);
	void ForceDriftFromExternalImpact();
	void SetExternalFrictionMultiplier(int32 WheelIndex, float Multiplier);
	void ApplySpeedCap(float MaxSpeed);
	void ClearSpeedCap();

private:
	float InitiatingTimer = 0.0f;
	float DriftingTimer = 0.0f;
	float CurrentSlipAngleDegrees = 0.0f;
	float CurrentGripBlendFactor = 1.0f;
	float LastLoggedGripBlendFactor = 1.0f;
	float CurrentRearFriction = 3.4f;
	float CurrentAppliedRearFriction = 3.4f;
	float FrozenThrottleValue = 0.0f;
	float FrozenBrakeValue = 0.0f;
	float FrozenSteeringValue = 0.0f;
	float TemporarySteeringMultiplier = 1.0f;
	float ActiveSpeedCap = 0.0f;
	float BaseFrontMaxSteerAngle = 40.0f;
	double LastSteeringScaleLogTime = -1.0;
	TArray<float> BaseWheelFrictionMultipliers;
	TArray<float> ExternalFrictionMultipliers;
	FTimerHandle InputFreezeRestoreTimerHandle;
	FTimerHandle TemporarySteeringMultiplierRestoreTimerHandle;
	bool bInputFrozen = false;
	bool bTemporarySteeringMultiplierActive = false;
	bool bLastHandbrakeState = false;
	bool bInitiatedByHandbrakeTap = false;
	bool bDriftAngleHasBuilt = false;
	bool bRecoveryAngleConfirmed = false;
	bool bHasPendingProfileValues = false;
	bool bSpeedCapActive = false;
	bool bSuppressThrottleOverrideForCurrentDrift = false;
	float PendingMaxSteerAngle = 40.0f;
	float PendingMaxBrakeTorque = 4500.0f;
	float PendingFrictionForceMultiplier = 3.0f;

	UCarPhysicsProfile* ResolvePhysicsProfile() const;
	void PrepareProfileValues();
	float ComputeBodySlipAngleDegrees() const;
	float GetBrakeInput();
	void UpdateDriftState(float DeltaTime, float BrakeInput, float SteeringInput, bool bHandbrakeActive);
	void SetDriftState(EHiveDriftState NewState);
	void ApplyDriftYawDamping();
	void UpdateGripBlend(float DeltaTime);
	void ApplyGripBlendToWheels();
	void LogGripBlendIfNeeded(float TargetGripBlendFactor);
	void UpdateRearFriction(float DeltaTime);
	float GetTargetRearFriction() const;
	float ComputeSpeedSensitiveSteeringScale(float AbsForwardSpeed) const;
	void UpdateSpeedSensitiveSteering();
	void RestoreTemporarySteeringMultiplier();
	void EnforceSpeedCap(bool bLogIfApplied);
	void RestoreFullInputFreeze();
};
