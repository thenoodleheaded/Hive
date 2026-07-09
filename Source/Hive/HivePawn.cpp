// Copyright Epic Games, Inc. All Rights Reserved.

#include "HivePawn.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "HiveSportsCar.h"
#include "HiveVehicleMovementComponent.h"
#include "InputActionValue.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "Hive.h"
#include "PowerUps/PowerUpBase.h"
#include "PowerUps/PowerUpSlotComponent.h"
#include "TimerManager.h"

AHivePawn::AHivePawn(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// construct the front camera boom
	FrontSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("Front Spring Arm"));
	FrontSpringArm->SetupAttachment(GetMesh());
	FrontSpringArm->TargetArmLength = 0.0f;
	FrontSpringArm->bDoCollisionTest = false;
	FrontSpringArm->bEnableCameraRotationLag = true;
	FrontSpringArm->CameraRotationLagSpeed = 15.0f;
	FrontSpringArm->SetRelativeLocation(FVector(30.0f, 0.0f, 120.0f));

	FrontCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Front Camera"));
	FrontCamera->SetupAttachment(FrontSpringArm);
	FrontCamera->bAutoActivate = false;

	// construct the back camera boom
	BackSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("Back Spring Arm"));
	BackSpringArm->SetupAttachment(GetMesh());
	BackSpringArm->TargetArmLength = 650.0f;
	BackSpringArm->SocketOffset.Z = 150.0f;
	BackSpringArm->bDoCollisionTest = false;
	BackSpringArm->bInheritPitch = false;
	BackSpringArm->bInheritRoll = false;
	BackSpringArm->bEnableCameraRotationLag = true;
	BackSpringArm->CameraRotationLagSpeed = 2.0f;
	BackSpringArm->CameraLagMaxDistance = 50.0f;

	BackCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Back Camera"));
	BackCamera->SetupAttachment(BackSpringArm);

	// Configure the car mesh
	GetMesh()->SetSimulatePhysics(true);
	GetMesh()->SetCollisionProfileName(FName("Vehicle"));

	// get the Chaos Wheeled movement component
	ChaosVehicleMovement = CastChecked<UChaosWheeledVehicleMovementComponent>(GetVehicleMovement());

}

void AHivePawn::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// steering 
		EnhancedInputComponent->BindAction(SteeringAction, ETriggerEvent::Triggered, this, &AHivePawn::Steering);
		EnhancedInputComponent->BindAction(SteeringAction, ETriggerEvent::Completed, this, &AHivePawn::Steering);

		// throttle 
		EnhancedInputComponent->BindAction(ThrottleAction, ETriggerEvent::Triggered, this, &AHivePawn::Throttle);
		EnhancedInputComponent->BindAction(ThrottleAction, ETriggerEvent::Completed, this, &AHivePawn::Throttle);

		// break 
		EnhancedInputComponent->BindAction(BrakeAction, ETriggerEvent::Triggered, this, &AHivePawn::Brake);
		EnhancedInputComponent->BindAction(BrakeAction, ETriggerEvent::Started, this, &AHivePawn::StartBrake);
		EnhancedInputComponent->BindAction(BrakeAction, ETriggerEvent::Completed, this, &AHivePawn::StopBrake);

		// handbrake 
		EnhancedInputComponent->BindAction(HandbrakeAction, ETriggerEvent::Started, this, &AHivePawn::StartHandbrake);
		EnhancedInputComponent->BindAction(HandbrakeAction, ETriggerEvent::Completed, this, &AHivePawn::StopHandbrake);

		// look around 
		EnhancedInputComponent->BindAction(LookAroundAction, ETriggerEvent::Triggered, this, &AHivePawn::LookAround);

		// toggle camera 
		EnhancedInputComponent->BindAction(ToggleCameraAction, ETriggerEvent::Triggered, this, &AHivePawn::ToggleCamera);

		// reset the vehicle 
		EnhancedInputComponent->BindAction(ResetVehicleAction, ETriggerEvent::Triggered, this, &AHivePawn::ResetVehicle);

		// activate power-up slot 1
		const UInputAction* ActivatePowerUpSlot1Action = LoadObject<UInputAction>(nullptr, TEXT("/Game/VehicleTemplate/Input/Actions/IA_ActivatePowerUp_Slot1.IA_ActivatePowerUp_Slot1"));
		if (ActivatePowerUpSlot1Action)
		{
			EnhancedInputComponent->BindActionValueLambda(ActivatePowerUpSlot1Action, ETriggerEvent::Started, [this](const FInputActionValue&)
			{
				if (AHiveSportsCar* SportsCar = Cast<AHiveSportsCar>(this))
				{
					SportsCar->ActivatePowerUp(0);
				}
			});
		}
		else
		{
			UE_LOG(LogHive, Warning, TEXT("IA_ActivatePowerUp_Slot1 was not found. Create it in /Game/VehicleTemplate/Input/Actions and add it to the active input mapping context to enable slot 1 activation."));
		}

	}
	else
	{
		UE_LOG(LogHive, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void AHivePawn::BeginPlay()
{
	Super::BeginPlay();

	// set up the flipped check timer
	GetWorld()->GetTimerManager().SetTimer(FlipCheckTimer, this, &AHivePawn::FlippedCheck, FlipCheckTime, true);

#if WITH_EDITOR
	AHiveSportsCar* SportsCar = Cast<AHiveSportsCar>(this);
	UPowerUpSlotComponent* PowerUpSlots = SportsCar ? SportsCar->GetPowerUpSlots() : nullptr;
	if (PowerUpSlots)
	{
		for (int32 DebugPowerUpIndex = 0; DebugPowerUpIndex < DebugStartingPowerUps.Num(); ++DebugPowerUpIndex)
		{
			TSubclassOf<APowerUpBase> PowerUpClass = DebugStartingPowerUps[DebugPowerUpIndex];
			if (!PowerUpClass)
			{
				continue;
			}

			int32 TargetSlotIndex = INDEX_NONE;
			for (int32 SlotIndex = 0; SlotIndex < PowerUpSlots->MaxSlots; ++SlotIndex)
			{
				if (!PowerUpSlots->GetSlotContent(SlotIndex))
				{
					TargetSlotIndex = SlotIndex;
					break;
				}
			}

			APowerUpBase* DebugPowerUp = GetWorld()->SpawnActor<APowerUpBase>(PowerUpClass, GetActorLocation(), GetActorRotation());
			if (!DebugPowerUp)
			{
				UE_LOG(LogHive, Warning, TEXT("[DebugPowerUps] Failed to spawn %s for %s."), *GetNameSafe(PowerUpClass.Get()), *GetNameSafe(this));
				continue;
			}

			const bool bAddedPowerUp = PowerUpSlots->TryAddPowerUp(DebugPowerUp);
			if (bAddedPowerUp)
			{
				UE_LOG(LogHive, Log, TEXT("[DebugPowerUps] Injected %s into slot %d on %s."), *GetNameSafe(DebugPowerUp), TargetSlotIndex, *GetNameSafe(this));
			}
			else
			{
				UE_LOG(LogHive, Warning, TEXT("[DebugPowerUps] Could not inject %s into %s because all power-up slots are full."), *GetNameSafe(DebugPowerUp), *GetNameSafe(this));
				DebugPowerUp->Destroy();
			}
		}
	}
#endif
}

void AHivePawn::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	// clear the flipped check timer
	GetWorld()->GetTimerManager().ClearTimer(FlipCheckTimer);

	Super::EndPlay(EndPlayReason);
}

void AHivePawn::Tick(float Delta)
{
	Super::Tick(Delta);

	// add some angular damping if the vehicle is in midair
	bool bMovingOnGround = ChaosVehicleMovement->IsMovingOnGround();
	GetMesh()->SetAngularDamping(bMovingOnGround ? 0.0f : 3.0f);

	// realign the camera yaw to face front
	const FRotator CurrentBackSpringArmRotation = BackSpringArm->GetRelativeRotation();
	float CameraYaw = CurrentBackSpringArmRotation.Yaw;
	CameraYaw = FMath::FInterpTo(CameraYaw, 0.0f, Delta, 1.0f);

	const UHiveVehicleMovementComponent* HiveMovement = Cast<UHiveVehicleMovementComponent>(GetChaosVehicleMovement());
	float TargetRoll = 0.0f;
	float TargetFOV = BaseFOV;

	if (HiveMovement)
	{
		const EHiveDriftState DriftState = HiveMovement->CurrentDriftState;
		const float CurrentSlipAngleDegrees = HiveMovement->GetCurrentSlipAngleDegrees();

		if (!bFrontCameraActive && (DriftState == EHiveDriftState::Drifting || DriftState == EHiveDriftState::Recovering))
		{
			const float LateralVelocity = FVector::DotProduct(GetVelocity(), GetActorRightVector());
			const float RollDirection = FMath::IsNearlyZero(LateralVelocity) ? 0.0f : FMath::Sign(LateralVelocity);
			const float SlipRollFraction = FMath::Clamp(CurrentSlipAngleDegrees / 25.0f, 0.0f, 1.0f);
			TargetRoll = MaxDriftRollDegrees * SlipRollFraction * RollDirection;
		}

		const float ForwardSpeed = FMath::Clamp(FVector::DotProduct(GetVelocity(), GetActorForwardVector()), 0.0f, TopSpeedReference);
		const float SpeedFraction = TopSpeedReference > KINDA_SMALL_NUMBER ? ForwardSpeed / TopSpeedReference : 0.0f;
		TargetFOV = FMath::Lerp(BaseFOV, MaxFOV, SpeedFraction);
	}

	const float CameraRoll = FMath::FInterpTo(CurrentBackSpringArmRotation.Roll, TargetRoll, Delta, DriftRollBlendSpeed);
	BackSpringArm->SetRelativeRotation(FRotator(CurrentBackSpringArmRotation.Pitch, CameraYaw, CameraRoll));

	const float CameraFOV = FMath::FInterpTo(BackCamera->FieldOfView, TargetFOV, Delta, FOVBlendSpeed);
	BackCamera->SetFieldOfView(CameraFOV);
}

void AHivePawn::Steering(const FInputActionValue& Value)
{
	// route the input
	DoSteering(Value.Get<float>());
}

void AHivePawn::Throttle(const FInputActionValue& Value)
{
	// route the input
	DoThrottle(Value.Get<float>());
}

void AHivePawn::Brake(const FInputActionValue& Value)
{
	// route the input
	DoBrake(Value.Get<float>());
}

void AHivePawn::StartBrake(const FInputActionValue& Value)
{
	// route the input
	DoBrakeStart();
}

void AHivePawn::StopBrake(const FInputActionValue& Value)
{
	// route the input
	DoBrakeStop();
}

void AHivePawn::StartHandbrake(const FInputActionValue& Value)
{
	// route the input
	DoHandbrakeStart();
}

void AHivePawn::StopHandbrake(const FInputActionValue& Value)
{
	// route the input
	DoHandbrakeStop();
}

void AHivePawn::LookAround(const FInputActionValue& Value)
{
	// route the input
	DoLookAround(Value.Get<float>());
}

void AHivePawn::ToggleCamera(const FInputActionValue& Value)
{
	// route the input
	DoToggleCamera();
}

void AHivePawn::ResetVehicle(const FInputActionValue& Value)
{
	// route the input
	DoResetVehicle();
}

void AHivePawn::DoSteering(float SteeringValue)
{
	if (const UHiveVehicleMovementComponent* HiveMovement = Cast<UHiveVehicleMovementComponent>(ChaosVehicleMovement))
	{
		if (HiveMovement->IsInputFrozen())
		{
			ChaosVehicleMovement->SetSteeringInput(HiveMovement->GetFrozenSteeringValue());
			ChaosVehicleMovement->SetThrottleInput(HiveMovement->GetFrozenThrottleValue());
			ChaosVehicleMovement->SetBrakeInput(HiveMovement->GetFrozenBrakeValue());
			return;
		}

		SteeringValue *= HiveMovement->GetTemporarySteeringMultiplier();
	}

	// add the input
	ChaosVehicleMovement->SetSteeringInput(SteeringValue);
}

void AHivePawn::DoThrottle(float ThrottleValue)
{
	if (const UHiveVehicleMovementComponent* HiveMovement = Cast<UHiveVehicleMovementComponent>(ChaosVehicleMovement))
	{
		if (HiveMovement->IsInputFrozen())
		{
			ChaosVehicleMovement->SetSteeringInput(HiveMovement->GetFrozenSteeringValue());
			ChaosVehicleMovement->SetThrottleInput(HiveMovement->GetFrozenThrottleValue());
			ChaosVehicleMovement->SetBrakeInput(HiveMovement->GetFrozenBrakeValue());
			return;
		}
	}

	// add the input
	ChaosVehicleMovement->SetThrottleInput(ThrottleValue);

	// reset the brake input
	ChaosVehicleMovement->SetBrakeInput(0.0f);
}

void AHivePawn::DoBrake(float BrakeValue)
{
	if (const UHiveVehicleMovementComponent* HiveMovement = Cast<UHiveVehicleMovementComponent>(ChaosVehicleMovement))
	{
		if (HiveMovement->IsInputFrozen())
		{
			ChaosVehicleMovement->SetSteeringInput(HiveMovement->GetFrozenSteeringValue());
			ChaosVehicleMovement->SetThrottleInput(HiveMovement->GetFrozenThrottleValue());
			ChaosVehicleMovement->SetBrakeInput(HiveMovement->GetFrozenBrakeValue());
			return;
		}
	}

	// add the input
	ChaosVehicleMovement->SetBrakeInput(BrakeValue);

	// reset the throttle input
	ChaosVehicleMovement->SetThrottleInput(0.0f);
}

void AHivePawn::DoBrakeStart()
{
	// call the Blueprint hook for the brake lights
	BrakeLights(true);
}

void AHivePawn::DoBrakeStop()
{
	// call the Blueprint hook for the brake lights
	BrakeLights(false);

	if (const UHiveVehicleMovementComponent* HiveMovement = Cast<UHiveVehicleMovementComponent>(ChaosVehicleMovement))
	{
		if (HiveMovement->IsInputFrozen())
		{
			ChaosVehicleMovement->SetSteeringInput(HiveMovement->GetFrozenSteeringValue());
			ChaosVehicleMovement->SetThrottleInput(HiveMovement->GetFrozenThrottleValue());
			ChaosVehicleMovement->SetBrakeInput(HiveMovement->GetFrozenBrakeValue());
			return;
		}
	}

	// reset brake input to zero
	ChaosVehicleMovement->SetBrakeInput(0.0f);
}

void AHivePawn::RefreshLiveInputState()
{
	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent);
	if (!EnhancedInputComponent || !ChaosVehicleMovement)
	{
		return;
	}

	float SteeringValue = SteeringAction ? EnhancedInputComponent->GetBoundActionValue(SteeringAction).Get<float>() : 0.0f;
	const float ThrottleValue = ThrottleAction ? EnhancedInputComponent->GetBoundActionValue(ThrottleAction).Get<float>() : 0.0f;
	const float BrakeValue = BrakeAction ? EnhancedInputComponent->GetBoundActionValue(BrakeAction).Get<float>() : 0.0f;

	if (const UHiveVehicleMovementComponent* HiveMovement = Cast<UHiveVehicleMovementComponent>(ChaosVehicleMovement))
	{
		SteeringValue *= HiveMovement->GetTemporarySteeringMultiplier();
	}

	ChaosVehicleMovement->SetSteeringInput(SteeringValue);
	ChaosVehicleMovement->SetThrottleInput(ThrottleValue);
	ChaosVehicleMovement->SetBrakeInput(BrakeValue);
}

void AHivePawn::DoHandbrakeStart()
{
	// add the input
	ChaosVehicleMovement->SetHandbrakeInput(true);

	// call the Blueprint hook for the break lights
	BrakeLights(true);
}

void AHivePawn::DoHandbrakeStop()
{
	// add the input
	ChaosVehicleMovement->SetHandbrakeInput(false);

	// call the Blueprint hook for the break lights
	BrakeLights(false);
}

void AHivePawn::DoLookAround(float YawDelta)
{
	// rotate the spring arm
	BackSpringArm->AddLocalRotation(FRotator(0.0f, YawDelta, 0.0f));
}

void AHivePawn::DoToggleCamera()
{
	// toggle the active camera flag
	bFrontCameraActive = !bFrontCameraActive;

	FrontCamera->SetActive(bFrontCameraActive);
	BackCamera->SetActive(!bFrontCameraActive);
}

void AHivePawn::DoResetVehicle()
{
	// reset to a location slightly above our current one
	FVector ResetLocation = GetActorLocation() + FVector(0.0f, 0.0f, 50.0f);

	// reset to our yaw. Ignore pitch and roll
	FRotator ResetRotation = GetActorRotation();
	ResetRotation.Pitch = 0.0f;
	ResetRotation.Roll = 0.0f;

	// teleport the actor to the reset spot and reset physics
	SetActorTransform(FTransform(ResetRotation, ResetLocation, FVector::OneVector), false, nullptr, ETeleportType::TeleportPhysics);

	GetMesh()->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	GetMesh()->SetPhysicsLinearVelocity(FVector::ZeroVector);
}

void AHivePawn::FlippedCheck()
{
	// check the difference in angle between the mesh's up vector and world up
	const float UpDot = FVector::DotProduct(FVector::UpVector, GetMesh()->GetUpVector());

	if (UpDot < FlipCheckMinDot)
	{
		// is this the second time we've checked that the vehicle is still flipped?
		if (bPreviousFlipCheck)
		{
			// reset the vehicle to upright
			DoResetVehicle();
		}
		
		// set the flipped check flag so the next check resets the car
		bPreviousFlipCheck = true;

	} else {

		// we're upright. reset the flipped check flag
		bPreviousFlipCheck = false;
	}
}
