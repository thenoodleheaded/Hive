// Copyright Epic Games, Inc. All Rights Reserved.

#include "HiveVehicleMovementComponent.h"
#include "Hive.h"
#include "HivePawn.h"
#include "HiveSportsCar.h"
#include "PhysicsEngine/BodyInstance.h"
#include "TimerManager.h"
#include "UCarPhysicsProfile.h"

namespace
{
	float MapProfileStat(float NormalisedStat, float MinValue, float MaxValue)
	{
		return FMath::Lerp(MinValue, MaxValue, FMath::Clamp(NormalisedStat, 0.0f, 1.0f));
	}

	const TCHAR* LexToString(EHiveDriftState DriftState)
	{
		switch (DriftState)
		{
		case EHiveDriftState::Gripping:
			return TEXT("Gripping");
		case EHiveDriftState::Initiating:
			return TEXT("Initiating");
		case EHiveDriftState::Drifting:
			return TEXT("Drifting");
		case EHiveDriftState::Recovering:
			return TEXT("Recovering");
		default:
			return TEXT("Unknown");
		}
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

	const float BaseMaxTorque = EngineSetup.MaxTorque;
	const float PendingTorqueMultiplier = MapProfileStat(AccelerationStat, 0.7f, 1.4f);
	EngineSetup.MaxTorque = BaseMaxTorque * PendingTorqueMultiplier;

	PendingMaxSteerAngle = MapProfileStat(HandlingStat, 30.0f, 50.0f);
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

	if (!PVehicle)
	{
		return;
	}

	BaseWheelFrictionMultipliers.Reset(PVehicle->Wheels.Num());
	ExternalFrictionMultipliers.Init(1.0f, PVehicle->Wheels.Num());

	const int32 ProfiledWheelCount = bHasPendingProfileValues ? FMath::Min(4, PVehicle->Wheels.Num()) : 0;
	for (int32 WheelIndex = 0; WheelIndex < PVehicle->Wheels.Num(); ++WheelIndex)
	{
		Chaos::FSimpleWheelSim& VehicleWheel = PVehicle->Wheels[WheelIndex];
		if (WheelIndex < ProfiledWheelCount)
		{
			VehicleWheel.MaxBrakeTorque = PendingMaxBrakeTorque;
			VehicleWheel.FrictionMultiplier = PendingFrictionForceMultiplier;

			if (WheelIndex < 2)
			{
				VehicleWheel.MaxSteeringAngle = PendingMaxSteerAngle;
			}
		}

		BaseWheelFrictionMultipliers.Add(VehicleWheel.FrictionMultiplier);
	}

	if (PVehicle->Wheels.Num() > 0)
	{
		BaseFrontMaxSteerAngle = PVehicle->Wheels[0].MaxSteeringAngle;
	}

	CurrentRearFriction = GetTargetRearFriction();
	CurrentAppliedRearFriction = CurrentRearFriction * CurrentGripBlendFactor;
}

void UHiveVehicleMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const float BrakeInputValue = GetBrakeInput();
	const float SteeringInputValue = GetSteeringInput();
	const bool bHandbrakeActive = GetHandbrakeInput();

	CurrentSlipAngleDegrees = ComputeBodySlipAngleDegrees();

	UpdateDriftState(DeltaTime, BrakeInputValue, SteeringInputValue, bHandbrakeActive);

	if (bOverrideThrottleDuringDrift && CurrentDriftState == EHiveDriftState::Drifting && !bSuppressThrottleOverrideForCurrentDrift)
	{
		SetThrottleInput(1.0f);
	}

	if (bEnableYawDamping)
	{
		ApplyDriftYawDamping();
	}
	UpdateGripBlend(DeltaTime);
	UpdateRearFriction(DeltaTime);
	UpdateSpeedSensitiveSteering();
	EnforceSpeedCap(false);

}

float UHiveVehicleMovementComponent::ComputeBodySlipAngleDegrees() const
{
	const USceneComponent* ReferenceComponent = UpdatedComponent ? UpdatedComponent.Get() : GetOwner() ? GetOwner()->GetRootComponent() : nullptr;
	if (!ReferenceComponent)
	{
		return 0.0f;
	}

	const FVector Velocity = ReferenceComponent->GetComponentVelocity();
	if (Velocity.SizeSquared() < 1.0f)
	{
		return 0.0f;
	}

	const float ForwardVelocity = FVector::DotProduct(Velocity, ReferenceComponent->GetForwardVector());
	const float LateralVelocity = FVector::DotProduct(Velocity, ReferenceComponent->GetRightVector());
	return FMath::RadiansToDegrees(FMath::Atan2(FMath::Abs(LateralVelocity), FMath::Abs(ForwardVelocity)));
}

float UHiveVehicleMovementComponent::GetBrakeInput()
{
	return UChaosWheeledVehicleMovementComponent::GetBrakeInput();
}

void UHiveVehicleMovementComponent::ApplyFullInputFreeze(float Duration)
{
	FrozenThrottleValue = GetThrottleInput();
	FrozenBrakeValue = GetBrakeInput();
	FrozenSteeringValue = GetSteeringInput();
	bInputFrozen = true;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(InputFreezeRestoreTimerHandle);
		World->GetTimerManager().SetTimer(InputFreezeRestoreTimerHandle, this, &UHiveVehicleMovementComponent::RestoreFullInputFreeze, Duration, false);
	}
}

void UHiveVehicleMovementComponent::ApplyTemporarySteeringMultiplier(float Multiplier, float Duration)
{
	TemporarySteeringMultiplier = FMath::Clamp(Multiplier, 0.0f, 1.0f);
	bTemporarySteeringMultiplierActive = Duration > 0.0f && TemporarySteeringMultiplier < 1.0f;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TemporarySteeringMultiplierRestoreTimerHandle);
		if (bTemporarySteeringMultiplierActive)
		{
			World->GetTimerManager().SetTimer(TemporarySteeringMultiplierRestoreTimerHandle, this, &UHiveVehicleMovementComponent::RestoreTemporarySteeringMultiplier, Duration, false);
		}
	}

	UE_LOG(LogHive, Verbose, TEXT("[SteeringReduction] %s multiplier %.2f applied for %.2fs."),
		*GetNameSafe(GetOwner()),
		TemporarySteeringMultiplier,
		Duration);
}

void UHiveVehicleMovementComponent::ForceDriftFromExternalImpact()
{
	SetDriftState(EHiveDriftState::Drifting);
	bSuppressThrottleOverrideForCurrentDrift = true;
}

void UHiveVehicleMovementComponent::SetExternalFrictionMultiplier(int32 WheelIndex, float Multiplier)
{
	if (!ExternalFrictionMultipliers.IsValidIndex(WheelIndex))
	{
		return;
	}

	ExternalFrictionMultipliers[WheelIndex] = FMath::Max(0.0f, Multiplier);
}

void UHiveVehicleMovementComponent::ApplySpeedCap(float MaxSpeed)
{
	ActiveSpeedCap = FMath::Max(0.0f, MaxSpeed);
	bSpeedCapActive = ActiveSpeedCap > KINDA_SMALL_NUMBER;
	EnforceSpeedCap(true);
}

void UHiveVehicleMovementComponent::ClearSpeedCap()
{
	if (bSpeedCapActive)
	{
		UE_LOG(LogHive, Verbose, TEXT("[SlowFieldCap] Cleared speed cap on %s."), *GetNameSafe(GetOwner()));
	}

	bSpeedCapActive = false;
	ActiveSpeedCap = 0.0f;
}

void UHiveVehicleMovementComponent::UpdateDriftState(float DeltaTime, float BrakeInputValue, float SteeringInputValue, bool bHandbrakeActive)
{
	const bool bBrakeDriftActive = BrakeInputValue > BrakeDriftThreshold;
	const bool bSteerDriftActive = FMath::Abs(SteeringInputValue) > SteerDriftThreshold;
	const bool bBrakeDriftTriggerActive = bBrakeDriftActive && FMath::Abs(SteeringInputValue) > MinSteerForBrakeDrift;
	const bool bHandbrakePressedThisTick = !bLastHandbrakeState && bHandbrakeActive;
	const bool bHandbrakeTapTriggerActive = bHandbrakePressedThisTick && bSteerDriftActive;
	const bool bMovingForwardForDrift = GetForwardSpeed() > MinForwardSpeedForDrift && GetTargetGear() >= 0;

	switch (CurrentDriftState)
	{
	case EHiveDriftState::Gripping:
		if (bMovingForwardForDrift && (bBrakeDriftTriggerActive || bHandbrakeTapTriggerActive))
		{
			bInitiatedByHandbrakeTap = bHandbrakeTapTriggerActive;
			SetDriftState(EHiveDriftState::Initiating);
		}
		break;

	case EHiveDriftState::Initiating:
		if (!bMovingForwardForDrift)
		{
			SetDriftState(EHiveDriftState::Gripping);
		}
		else if (!bInitiatedByHandbrakeTap && !bBrakeDriftActive)
		{
			SetDriftState(EHiveDriftState::Gripping);
		}
		else
		{
			InitiatingTimer += DeltaTime;
			if (InitiatingTimer >= InitiatingHoldTime)
			{
				SetDriftState(EHiveDriftState::Drifting);
			}
		}
		break;

	case EHiveDriftState::Drifting:
		DriftingTimer += DeltaTime;

		if (CurrentSlipAngleDegrees >= SlipAngleDriftExitThreshold)
		{
			bDriftAngleHasBuilt = true;
		}

		if (bDriftAngleHasBuilt && CurrentSlipAngleDegrees < SlipAngleDriftExitThreshold)
		{
			SetDriftState(EHiveDriftState::Recovering);
		}
		else if (DriftingTimer > DriftNoSlipTimeout && !bDriftAngleHasBuilt)
		{
			SetDriftState(EHiveDriftState::Recovering);
		}
		break;

	case EHiveDriftState::Recovering:
		if (CurrentSlipAngleDegrees < SlipAngleRecoveryThreshold)
		{
			bRecoveryAngleConfirmed = true;
		}

		if (bRecoveryAngleConfirmed)
		{
			SetDriftState(EHiveDriftState::Gripping);
		}
		break;

	default:
		SetDriftState(EHiveDriftState::Gripping);
		break;
	}

	bLastHandbrakeState = bHandbrakeActive;
}

void UHiveVehicleMovementComponent::SetDriftState(EHiveDriftState NewState)
{
	if (CurrentDriftState == NewState)
	{
		return;
	}

	const EHiveDriftState OldState = CurrentDriftState;
	CurrentDriftState = NewState;

	if (CurrentDriftState == EHiveDriftState::Initiating)
	{
		InitiatingTimer = 0.0f;
	}
	else if (CurrentDriftState == EHiveDriftState::Drifting)
	{
		DriftingTimer = 0.0f;
		bDriftAngleHasBuilt = false;
	}
	else if (CurrentDriftState == EHiveDriftState::Recovering)
	{
		bRecoveryAngleConfirmed = false;
	}
	else
	{
		bInitiatedByHandbrakeTap = false;
	}

	if (CurrentDriftState != EHiveDriftState::Drifting)
	{
		bSuppressThrottleOverrideForCurrentDrift = false;
	}

	UE_LOG(LogHive, Verbose, TEXT("[DriftFSM] State change: %s -> %s at Brake=%.2f, Steering=%.2f, Handbrake=%s"),
		LexToString(OldState),
		LexToString(CurrentDriftState),
		GetBrakeInput(),
		GetSteeringInput(),
		GetHandbrakeInput() ? TEXT("true") : TEXT("false"));
}

void UHiveVehicleMovementComponent::ApplyDriftYawDamping()
{
	if (CurrentDriftState != EHiveDriftState::Drifting && CurrentDriftState != EHiveDriftState::Recovering)
	{
		return;
	}

	if (YawDampingStrength <= 0.0f)
	{
		return;
	}

	UPrimitiveComponent* PhysicsComponent = Cast<UPrimitiveComponent>(UpdatedComponent ? UpdatedComponent.Get() : nullptr);
	if (!PhysicsComponent)
	{
		PhysicsComponent = Cast<UPrimitiveComponent>(GetOwner() ? GetOwner()->GetRootComponent() : nullptr);
	}

	FBodyInstance* BodyInstance = PhysicsComponent ? PhysicsComponent->GetBodyInstance() : nullptr;
	if (!BodyInstance)
	{
		return;
	}

	const FVector UpAxis = PhysicsComponent->GetUpVector();
	const FVector AngularVelocity = BodyInstance->GetUnrealWorldAngularVelocityInRadians();
	const float YawRate = FVector::DotProduct(AngularVelocity, UpAxis);
	const FVector RawDampingTorque = -UpAxis * YawRate * YawDampingStrength;
	const FVector DampingTorque = RawDampingTorque * YawDampingDebugScale;
	const float AppliedYawTorque = FVector::DotProduct(DampingTorque, UpAxis);

	BodyInstance->AddTorqueInRadians(DampingTorque, false, true);

	UE_LOG(LogHive, Verbose, TEXT("[DriftYawDamping] YawRate=%.3f rad/s Sign=%+.0f, AppliedYawTorque=%.3f Sign=%+.0f, DebugScale=%.3f, DampingTorque=%s"),
		YawRate,
		FMath::Sign(YawRate),
		AppliedYawTorque,
		FMath::Sign(AppliedYawTorque),
		YawDampingDebugScale,
		*DampingTorque.ToCompactString());
}

void UHiveVehicleMovementComponent::UpdateGripBlend(float DeltaTime)
{
	const float TargetGripBlendFactor = CurrentDriftState == EHiveDriftState::Drifting ? DriftGripMultiplier : 1.0f;
	CurrentGripBlendFactor = FMath::FInterpTo(CurrentGripBlendFactor, TargetGripBlendFactor, DeltaTime, GripBlendSpeed);

	ApplyGripBlendToWheels();
	LogGripBlendIfNeeded(TargetGripBlendFactor);
}

void UHiveVehicleMovementComponent::ApplyGripBlendToWheels()
{
	if (BaseWheelFrictionMultipliers.IsEmpty())
	{
		return;
	}

	for (int32 WheelIndex = 0; WheelIndex < BaseWheelFrictionMultipliers.Num(); ++WheelIndex)
	{
		const bool bProtectFrontGrip = CurrentDriftState == EHiveDriftState::Drifting && WheelIndex < 2;
		const float BaseAppliedFriction = bProtectFrontGrip ? BaseWheelFrictionMultipliers[WheelIndex] : BaseWheelFrictionMultipliers[WheelIndex] * CurrentGripBlendFactor;
		const float ExternalMultiplier = ExternalFrictionMultipliers.IsValidIndex(WheelIndex) ? ExternalFrictionMultipliers[WheelIndex] : 1.0f;
		const float AppliedFriction = BaseAppliedFriction * ExternalMultiplier;
		SetWheelFrictionMultiplier(WheelIndex, AppliedFriction);
	}
}

void UHiveVehicleMovementComponent::LogGripBlendIfNeeded(float TargetGripBlendFactor)
{
	const bool bSignificantBlendChange = FMath::Abs(CurrentGripBlendFactor - LastLoggedGripBlendFactor) >= 0.05f;
	const bool bReachedBlendTarget = FMath::IsNearlyEqual(CurrentGripBlendFactor, TargetGripBlendFactor, 0.005f) && !FMath::IsNearlyEqual(LastLoggedGripBlendFactor, TargetGripBlendFactor, 0.005f);

	if (!bSignificantBlendChange && !bReachedBlendTarget)
	{
		return;
	}

	UE_LOG(LogHive, Verbose, TEXT("[DriftGrip] State=%s, GripBlend=%.2f, TargetGripBlend=%.2f, BaseFriction=%.2f"),
		LexToString(CurrentDriftState),
		CurrentGripBlendFactor,
		TargetGripBlendFactor,
		BaseWheelFrictionMultipliers.IsValidIndex(0) ? BaseWheelFrictionMultipliers[0] : 0.0f);

	LastLoggedGripBlendFactor = CurrentGripBlendFactor;
}

void UHiveVehicleMovementComponent::UpdateRearFriction(float DeltaTime)
{
	if (BaseWheelFrictionMultipliers.Num() <= 3)
	{
		return;
	}

	CurrentRearFriction = FMath::FInterpTo(CurrentRearFriction, GetTargetRearFriction(), DeltaTime, GripBlendSpeed);
	CurrentAppliedRearFriction = CurrentRearFriction * CurrentGripBlendFactor;

	const float RearLeftExternalMultiplier = ExternalFrictionMultipliers.IsValidIndex(2) ? ExternalFrictionMultipliers[2] : 1.0f;
	const float RearRightExternalMultiplier = ExternalFrictionMultipliers.IsValidIndex(3) ? ExternalFrictionMultipliers[3] : 1.0f;

	SetWheelFrictionMultiplier(2, CurrentAppliedRearFriction * RearLeftExternalMultiplier);
	SetWheelFrictionMultiplier(3, CurrentAppliedRearFriction * RearRightExternalMultiplier);
}

float UHiveVehicleMovementComponent::GetTargetRearFriction() const
{
	switch (CurrentDriftState)
	{
	case EHiveDriftState::Initiating:
		return (GrippingRearFriction + DriftingRearFriction) * 0.5f;
	case EHiveDriftState::Drifting:
	case EHiveDriftState::Recovering:
	{
		const float SlipAngleBlendFactor = FMath::Clamp(CurrentSlipAngleDegrees / FMath::Max(SlipAngleFullDriftDegrees, KINDA_SMALL_NUMBER), 0.0f, 1.0f);
		return FMath::Lerp(GrippingRearFriction, DriftingRearFriction, SlipAngleBlendFactor);
	}
	case EHiveDriftState::Gripping:
	default:
		return GrippingRearFriction;
	}
}

float UHiveVehicleMovementComponent::ComputeSpeedSensitiveSteeringScale(float AbsForwardSpeed) const
{
	const float SafeTopSpeedReference = FMath::Max(SteeringTopSpeedReference, 1.0f);
	const float SpeedAlpha = FMath::Clamp(AbsForwardSpeed / SafeTopSpeedReference, 0.0f, 1.0f);
	return FMath::Lerp(MinSteeringSpeedScale, MaxSteeringSpeedScale, SpeedAlpha);
}

void UHiveVehicleMovementComponent::UpdateSpeedSensitiveSteering()
{
	const float AbsForwardSpeed = FMath::Abs(GetForwardSpeed());
	const float SteeringScale = ComputeSpeedSensitiveSteeringScale(AbsForwardSpeed);
	const float EffectiveSteerAngle = BaseFrontMaxSteerAngle * SteeringScale;

	SetWheelMaxSteerAngle(0, EffectiveSteerAngle);
	SetWheelMaxSteerAngle(1, EffectiveSteerAngle);

	UWorld* World = GetWorld();
	const double CurrentTime = World ? World->GetTimeSeconds() : 0.0;
	if (!World || LastSteeringScaleLogTime < 0.0 || CurrentTime - LastSteeringScaleLogTime >= 0.5)
	{
		UE_LOG(LogHive, Verbose, TEXT("[SpeedSteering] %s Speed=%.1f Scale=%.2f BaseAngle=%.1f EffectiveAngle=%.1f"),
			*GetNameSafe(GetOwner()),
			AbsForwardSpeed,
			SteeringScale,
			BaseFrontMaxSteerAngle,
			EffectiveSteerAngle);

		LastSteeringScaleLogTime = CurrentTime;
	}
}

void UHiveVehicleMovementComponent::RestoreTemporarySteeringMultiplier()
{
	bTemporarySteeringMultiplierActive = false;
	TemporarySteeringMultiplier = 1.0f;

	UE_LOG(LogHive, Verbose, TEXT("[SteeringReduction] %s steering multiplier restored."),
		*GetNameSafe(GetOwner()));
}

void UHiveVehicleMovementComponent::EnforceSpeedCap(bool bLogIfApplied)
{
	if (!bSpeedCapActive)
	{
		return;
	}

	UPrimitiveComponent* PhysicsComponent = Cast<UPrimitiveComponent>(UpdatedComponent ? UpdatedComponent.Get() : nullptr);
	if (!PhysicsComponent)
	{
		PhysicsComponent = Cast<UPrimitiveComponent>(GetOwner() ? GetOwner()->GetRootComponent() : nullptr);
	}

	FBodyInstance* BodyInstance = PhysicsComponent ? PhysicsComponent->GetBodyInstance() : nullptr;
	if (!BodyInstance)
	{
		return;
	}

	const FVector CurrentVelocity = BodyInstance->GetUnrealWorldVelocity();
	const float CurrentSpeed = CurrentVelocity.Size();
	if (CurrentSpeed <= ActiveSpeedCap || CurrentSpeed <= KINDA_SMALL_NUMBER)
	{
		if (bLogIfApplied)
		{
			UE_LOG(LogHive, Verbose, TEXT("[SlowFieldCap] %s speed %.1f is already under cap %.1f."),
				*GetNameSafe(GetOwner()),
				CurrentSpeed,
				ActiveSpeedCap);
		}
		return;
	}

	const FVector CappedVelocity = CurrentVelocity.GetSafeNormal() * ActiveSpeedCap;
	BodyInstance->SetLinearVelocity(CappedVelocity, false);

	if (bLogIfApplied)
	{
		UE_LOG(LogHive, Verbose, TEXT("[SlowFieldCap] %s speed %.1f capped to %.1f. Velocity=%s -> %s"),
			*GetNameSafe(GetOwner()),
			CurrentSpeed,
			ActiveSpeedCap,
			*CurrentVelocity.ToCompactString(),
			*CappedVelocity.ToCompactString());
	}
}

void UHiveVehicleMovementComponent::RestoreFullInputFreeze()
{
	bInputFrozen = false;

	if (AHivePawn* HivePawn = Cast<AHivePawn>(GetOwner()))
	{
		HivePawn->RefreshLiveInputState();
	}
}
