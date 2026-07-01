// Copyright Epic Games, Inc. All Rights Reserved.

#include "HiveVehicleMovementComponent.h"
#include "Hive.h"
#include "HiveSportsCar.h"
#include "Engine/Engine.h"
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

	if (bOverrideThrottleDuringDrift && CurrentDriftState == EHiveDriftState::Drifting)
	{
		SetThrottleInput(1.0f);
	}

	UpdateGripBlend(DeltaTime);
	UpdateRearFriction(DeltaTime);

	if (GEngine)
	{
		const FString InitiatingTimerText = CurrentDriftState == EHiveDriftState::Initiating ? FString::Printf(TEXT("%.2f"), InitiatingTimer) : FString();
		GEngine->AddOnScreenDebugMessage(
			482001,
			0.0f,
			FColor::Cyan,
			FString::Printf(TEXT("Drift=%s | Brake=%.1f | Steer=%.1f | GripBlend=%.2f | RearFric=%.2f | Init=%s"),
				LexToString(CurrentDriftState),
				BrakeInputValue,
				SteeringInputValue,
				CurrentGripBlendFactor,
				CurrentAppliedRearFriction,
				*InitiatingTimerText));
	}
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

void UHiveVehicleMovementComponent::UpdateDriftState(float DeltaTime, float BrakeInputValue, float SteeringInputValue, bool bHandbrakeActive)
{
	const bool bBrakeDriftActive = BrakeInputValue > BrakeDriftThreshold;
	const bool bSteerDriftActive = FMath::Abs(SteeringInputValue) > SteerDriftThreshold;
	const bool bBrakeDriftTriggerActive = bBrakeDriftActive && FMath::Abs(SteeringInputValue) > MinSteerForBrakeDrift;
	const bool bHandbrakePressedThisTick = !bLastHandbrakeState && bHandbrakeActive;
	const bool bHandbrakeTapTriggerActive = bHandbrakePressedThisTick && bSteerDriftActive;

	switch (CurrentDriftState)
	{
	case EHiveDriftState::Gripping:
		if (bBrakeDriftTriggerActive || bHandbrakeTapTriggerActive)
		{
			bInitiatedByHandbrakeTap = bHandbrakeTapTriggerActive;
			SetDriftState(EHiveDriftState::Initiating);
		}
		break;

	case EHiveDriftState::Initiating:
		if (!bInitiatedByHandbrakeTap && !bBrakeDriftActive)
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

	UE_LOG(LogHive, Log, TEXT("[DriftFSM] State change: %s -> %s at Brake=%.2f, Steering=%.2f, Handbrake=%s"),
		LexToString(OldState),
		LexToString(CurrentDriftState),
		GetBrakeInput(),
		GetSteeringInput(),
		GetHandbrakeInput() ? TEXT("true") : TEXT("false"));
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

	CurrentAppliedFrictionMultiplier = 0.0f;

	for (int32 WheelIndex = 0; WheelIndex < BaseWheelFrictionMultipliers.Num(); ++WheelIndex)
	{
		const bool bProtectFrontGrip = CurrentDriftState == EHiveDriftState::Drifting && WheelIndex < 2;
		const float AppliedFriction = bProtectFrontGrip ? BaseWheelFrictionMultipliers[WheelIndex] : BaseWheelFrictionMultipliers[WheelIndex] * CurrentGripBlendFactor;
		SetWheelFrictionMultiplier(WheelIndex, AppliedFriction);

		if (WheelIndex == 0)
		{
			CurrentAppliedFrictionMultiplier = AppliedFriction;
		}
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

	UE_LOG(LogHive, Log, TEXT("[DriftGrip] State=%s, GripBlend=%.2f, TargetGripBlend=%.2f, BaseFriction=%.2f, AppliedFriction=%.2f"),
		LexToString(CurrentDriftState),
		CurrentGripBlendFactor,
		TargetGripBlendFactor,
		BaseWheelFrictionMultipliers.IsValidIndex(0) ? BaseWheelFrictionMultipliers[0] : 0.0f,
		CurrentAppliedFrictionMultiplier);

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

	SetWheelFrictionMultiplier(2, CurrentAppliedRearFriction);
	SetWheelFrictionMultiplier(3, CurrentAppliedRearFriction);
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
