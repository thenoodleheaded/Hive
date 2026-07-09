// Copyright Epic Games, Inc. All Rights Reserved.


#include "HiveSportsCar.h"
#include "Components/SphereComponent.h"
#include "HiveSportsWheelFront.h"
#include "HiveSportsWheelRear.h"
#include "HiveVehicleMovementComponent.h"
#include "PowerUps/MirrorShield.h"
#include "PowerUps/PowerUpSlotComponent.h"
#include "PowerUps/Shield.h"
#include "WheeledVehiclePawn.h"

AHiveSportsCar::AHiveSportsCar(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UHiveVehicleMovementComponent>(AWheeledVehiclePawn::VehicleMovementComponentName))
{
	SyncPhysicsProfileToMovementComponent();

	PowerUpSlots = CreateDefaultSubobject<UPowerUpSlotComponent>(TEXT("PowerUpSlots"));

	GameplayTriggerVolume = CreateDefaultSubobject<USphereComponent>(TEXT("GameplayTriggerVolume"));
	GameplayTriggerVolume->SetupAttachment(GetRootComponent());
	GameplayTriggerVolume->InitSphereRadius(150.0f);
	GameplayTriggerVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	GameplayTriggerVolume->SetCollisionObjectType(ECC_Vehicle);
	GameplayTriggerVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	GameplayTriggerVolume->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	GameplayTriggerVolume->SetGenerateOverlapEvents(true);
	GameplayTriggerVolume->SetCanEverAffectNavigation(false);

	// Note: for faster iteration times, the vehicle setup can be tweaked in the Blueprint instead

	// Set up the chassis
	GetChaosVehicleMovement()->ChassisHeight = 110.0f;
	GetChaosVehicleMovement()->DragCoefficient = 0.31f;

	// Set up the wheels
	GetChaosVehicleMovement()->bLegacyWheelFrictionPosition = true;
	GetChaosVehicleMovement()->WheelSetups.SetNum(4);

	GetChaosVehicleMovement()->WheelSetups[0].WheelClass = UHiveSportsWheelFront::StaticClass();
	GetChaosVehicleMovement()->WheelSetups[0].BoneName = FName("Phys_Wheel_FL");
	GetChaosVehicleMovement()->WheelSetups[0].AdditionalOffset = FVector(0.0f, 0.0f, 0.0f);

	GetChaosVehicleMovement()->WheelSetups[1].WheelClass = UHiveSportsWheelFront::StaticClass();
	GetChaosVehicleMovement()->WheelSetups[1].BoneName = FName("Phys_Wheel_FR");
	GetChaosVehicleMovement()->WheelSetups[1].AdditionalOffset = FVector(0.0f, 0.0f, 0.0f);

	GetChaosVehicleMovement()->WheelSetups[2].WheelClass = UHiveSportsWheelRear::StaticClass();
	GetChaosVehicleMovement()->WheelSetups[2].BoneName = FName("Phys_Wheel_BL");
	GetChaosVehicleMovement()->WheelSetups[2].AdditionalOffset = FVector(0.0f, 0.0f, 0.0f);

	GetChaosVehicleMovement()->WheelSetups[3].WheelClass = UHiveSportsWheelRear::StaticClass();
	GetChaosVehicleMovement()->WheelSetups[3].BoneName = FName("Phys_Wheel_BR");
	GetChaosVehicleMovement()->WheelSetups[3].AdditionalOffset = FVector(0.0f, 0.0f, 0.0f);

	// Set up the engine
	// NOTE: Check the Blueprint asset for the Torque Curve
	GetChaosVehicleMovement()->EngineSetup.MaxTorque = 1100.0f;
	GetChaosVehicleMovement()->EngineSetup.MaxRPM = 7000.0f;
	GetChaosVehicleMovement()->EngineSetup.EngineIdleRPM = 900.0f;
	GetChaosVehicleMovement()->EngineSetup.EngineBrakeEffect = 0.2f;
	GetChaosVehicleMovement()->EngineSetup.EngineRevUpMOI = 5.0f;
	GetChaosVehicleMovement()->EngineSetup.EngineRevDownRate = 600.0f;

	// Set up the transmission
	GetChaosVehicleMovement()->TransmissionSetup.bUseAutomaticGears = true;
	GetChaosVehicleMovement()->TransmissionSetup.bUseAutoReverse = true;
	GetChaosVehicleMovement()->TransmissionSetup.FinalRatio = 2.81f;
	GetChaosVehicleMovement()->TransmissionSetup.ChangeUpRPM = 6000.0f;
	GetChaosVehicleMovement()->TransmissionSetup.ChangeDownRPM = 2000.0f;
	GetChaosVehicleMovement()->TransmissionSetup.GearChangeTime = 0.2f;
	GetChaosVehicleMovement()->TransmissionSetup.TransmissionEfficiency = 0.9f;

	GetChaosVehicleMovement()->TransmissionSetup.ForwardGearRatios.SetNum(5);
	GetChaosVehicleMovement()->TransmissionSetup.ForwardGearRatios[0] = 4.25f;
	GetChaosVehicleMovement()->TransmissionSetup.ForwardGearRatios[1] = 2.52f;
	GetChaosVehicleMovement()->TransmissionSetup.ForwardGearRatios[2] = 1.66f;
	GetChaosVehicleMovement()->TransmissionSetup.ForwardGearRatios[3] = 1.22f;
	GetChaosVehicleMovement()->TransmissionSetup.ForwardGearRatios[4] = 1.0f;

	GetChaosVehicleMovement()->TransmissionSetup.ReverseGearRatios.SetNum(1);
	GetChaosVehicleMovement()->TransmissionSetup.ReverseGearRatios[0] = 4.04f;

	// Set up the steering
	// NOTE: Check the Blueprint asset for the Steering Curve
	GetChaosVehicleMovement()->SteeringSetup.SteeringType = ESteeringType::SingleAngle;
	GetChaosVehicleMovement()->SteeringSetup.AngleRatio = 1.0f;
	GetChaosVehicleMovement()->SteeringInputRate.RiseRate = 100.0f;
	GetChaosVehicleMovement()->SteeringInputRate.FallRate = 100.0f;
	GetChaosVehicleMovement()->SteeringInputRate.InputCurveFunction = EInputFunctionType::LinearFunction;
}

void AHiveSportsCar::SyncPhysicsProfileToMovementComponent() const
{
	if (UHiveVehicleMovementComponent* HiveMovement = Cast<UHiveVehicleMovementComponent>(GetChaosVehicleMovement()))
	{
		HiveMovement->PhysicsProfile = PhysicsProfile;
	}
}

void AHiveSportsCar::ActivatePowerUp(int32 SlotIndex)
{
	if (PowerUpSlots)
	{
		PowerUpSlots->ActivateSlot(SlotIndex);
	}
}

bool AHiveSportsCar::TryAbsorbHit()
{
	return bShieldActive && ActiveShield ? ActiveShield->TryAbsorbHit() : false;
}

bool AHiveSportsCar::TryReflectProjectile(AActor* Projectile)
{
	return bMirrorShieldActive && ActiveMirrorShield ? ActiveMirrorShield->TryReflectProjectile(Projectile) : false;
}

void AHiveSportsCar::SetActiveShield(AShield* Shield)
{
	ActiveShield = Shield;
	bShieldActive = IsValid(Shield);
}

void AHiveSportsCar::ClearActiveShield(AShield* Shield)
{
	if (ActiveShield == Shield)
	{
		ActiveShield = nullptr;
		bShieldActive = false;
	}
}

void AHiveSportsCar::SetActiveMirrorShield(AMirrorShield* MirrorShield)
{
	ActiveMirrorShield = MirrorShield;
	bMirrorShieldActive = IsValid(MirrorShield);
}

void AHiveSportsCar::ClearActiveMirrorShield(AMirrorShield* MirrorShield)
{
	if (ActiveMirrorShield == MirrorShield)
	{
		ActiveMirrorShield = nullptr;
		bMirrorShieldActive = false;
	}
}
