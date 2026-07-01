// Copyright Epic Games, Inc. All Rights Reserved.

#include "HiveVehicleMovementComponent.h"
#include "Hive.h"
#include "HiveSportsCar.h"
#include "PhysicsEngine/BodyInstance.h"
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

	const float PendingTorqueMultiplier = MapProfileStat(AccelerationStat, 0.7f, 1.4f);
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

void UHiveVehicleMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const float LateralVelocity = GetSignedLateralVelocity();
	const float SteeringInputValue = GetSteeringInput();
	const float ThrottleInputValue = GetThrottleInput();
	const bool bHandbrakeActive = GetHandbrakeInput();

	UpdateDriftState(LateralVelocity, SteeringInputValue, ThrottleInputValue, bHandbrakeActive);
	ApplyDriftVelocityManipulation(DeltaTime);
}

float UHiveVehicleMovementComponent::GetSignedLateralVelocity() const
{
	const USceneComponent* ReferenceComponent = UpdatedComponent ? UpdatedComponent.Get() : GetOwner() ? GetOwner()->GetRootComponent() : nullptr;
	if (!ReferenceComponent)
	{
		return 0.0f;
	}

	return FVector::DotProduct(ReferenceComponent->GetComponentVelocity(), ReferenceComponent->GetRightVector());
}

void UHiveVehicleMovementComponent::UpdateDriftState(float LateralVelocity, float SteeringInputValue, float ThrottleInputValue, bool bHandbrakeActive)
{
	const float AbsLateralVelocity = FMath::Abs(LateralVelocity);
	const bool bSteerThrottleInitiation = FMath::Abs(SteeringInputValue) > SteeringDriftThreshold && ThrottleInputValue > ThrottleDriftThreshold;
	const bool bInitiationStillActive = bHandbrakeActive || bSteerThrottleInitiation;

	switch (CurrentDriftState)
	{
	case EHiveDriftState::Gripping:
		if (bInitiationStillActive)
		{
			SetDriftState(EHiveDriftState::Initiating, LateralVelocity, SteeringInputValue, ThrottleInputValue);
		}
		break;

	case EHiveDriftState::Initiating:
		if (AbsLateralVelocity > LateralVelocityDriftThreshold)
		{
			SetDriftState(EHiveDriftState::Drifting, LateralVelocity, SteeringInputValue, ThrottleInputValue);
		}
		else if (!bInitiationStillActive)
		{
			SetDriftState(EHiveDriftState::Gripping, LateralVelocity, SteeringInputValue, ThrottleInputValue);
		}
		break;

	case EHiveDriftState::Drifting:
		if (AbsLateralVelocity < LateralVelocityRecoveryThreshold)
		{
			SetDriftState(EHiveDriftState::Recovering, LateralVelocity, SteeringInputValue, ThrottleInputValue);
		}
		break;

	case EHiveDriftState::Recovering:
		if (AbsLateralVelocity < LateralVelocityRecoveryThreshold)
		{
			SetDriftState(EHiveDriftState::Gripping, LateralVelocity, SteeringInputValue, ThrottleInputValue);
		}
		break;

	default:
		SetDriftState(EHiveDriftState::Gripping, LateralVelocity, SteeringInputValue, ThrottleInputValue);
		break;
	}

	++DriftStateTickCount;
}

void UHiveVehicleMovementComponent::SetDriftState(EHiveDriftState NewState, float LateralVelocity, float SteeringInputValue, float ThrottleInputValue)
{
	if (CurrentDriftState == NewState)
	{
		return;
	}

	const EHiveDriftState OldState = CurrentDriftState;
	CurrentDriftState = NewState;
	DriftStateTickCount = 0;

	if (OldState == EHiveDriftState::Initiating && CurrentDriftState == EHiveDriftState::Drifting)
	{
		DriftEntrySpeed = FMath::Abs(GetForwardSpeed());
		DriftTargetRetainedSpeed = 0.0f;
		DriftActualForwardSpeed = DriftEntrySpeed;
	}

	UE_LOG(LogHive, Log, TEXT("[DriftFSM] State change: %s -> %s at LateralVel=%.1f, Steering=%.2f, Throttle=%.2f"),
		LexToString(OldState),
		LexToString(CurrentDriftState),
		LateralVelocity,
		SteeringInputValue,
		ThrottleInputValue);
}

void UHiveVehicleMovementComponent::ApplyDriftVelocityManipulation(float DeltaTime)
{
	if (CurrentDriftState != EHiveDriftState::Drifting)
	{
		return;
	}

	FBodyInstance* TargetInstance = GetBodyInstance();
	const USceneComponent* ReferenceComponent = UpdatedComponent ? UpdatedComponent.Get() : GetOwner() ? GetOwner()->GetRootComponent() : nullptr;
	if (!TargetInstance || !ReferenceComponent)
	{
		return;
	}

	const FVector ForwardVector = ReferenceComponent->GetForwardVector();
	const FVector RightVector = ReferenceComponent->GetRightVector();
	const FVector CurrentVelocity = ReferenceComponent->GetComponentVelocity();

	const float CurrentForwardSpeed = FVector::DotProduct(CurrentVelocity, ForwardVector);
	const float CurrentLateralSpeed = FVector::DotProduct(CurrentVelocity, RightVector);
	const FVector ForwardComponent = ForwardVector * CurrentForwardSpeed;
	const FVector LateralComponent = RightVector * CurrentLateralSpeed;
	const FVector RemainingVelocity = CurrentVelocity - ForwardComponent - LateralComponent;

	const float RetentionAlpha = (DriftSpeedRetentionMin + DriftSpeedRetentionMax) * 0.5f;
	DriftTargetRetainedSpeed = DriftEntrySpeed * RetentionAlpha;
	DriftActualForwardSpeed = FMath::Abs(CurrentForwardSpeed);

	const float ForwardDirection = FMath::Abs(CurrentForwardSpeed) > KINDA_SMALL_NUMBER ? FMath::Sign(CurrentForwardSpeed) : 1.0f;
	const float TargetForwardSpeed = DriftTargetRetainedSpeed * ForwardDirection;
	const float AdjustedForwardSpeed = FMath::FInterpTo(CurrentForwardSpeed, TargetForwardSpeed, DeltaTime, SpeedRetentionLerpSpeed);
	const float AdjustedLateralSpeed = CurrentLateralSpeed * LateralVelocitySustainFactor;

	const FVector AdjustedVelocity = (ForwardVector * AdjustedForwardSpeed) + (RightVector * AdjustedLateralSpeed) + RemainingVelocity;
	TargetInstance->SetLinearVelocity(AdjustedVelocity, false);
}
