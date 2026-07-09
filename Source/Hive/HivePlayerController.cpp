// Copyright Epic Games, Inc. All Rights Reserved.


#include "HivePlayerController.h"
#include "HivePawn.h"
#include "HiveUI.h"
#include "EnhancedInputSubsystems.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "Blueprint/UserWidget.h"
#include "Hive.h"
#include "HiveSportsCar.h"
#include "HiveVehicleMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "PowerUps/PowerUpSlotComponent.h"
#include "Widgets/Input/SVirtualJoystick.h"

void AHivePlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	// ensure we're attached to the vehicle pawn so that World Partition streaming works correctly
	bAttachToPawn = true;
}

void AHivePlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	
	// only add IMCs for local player controllers
	if (IsLocalPlayerController())
	{
		// Add Input Mapping Contexts
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}

			// only add these IMCs if we're not using mobile touch input
			if (!ShouldUseTouchControls())
			{
				for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
				{
					Subsystem->AddMappingContext(CurrentContext, 0);
				}
			}

			if (bUseSteeringWheelControls)
			{
				Subsystem->AddMappingContext(SteeringWheelInputMappingContext, 0);
			}
		}
	}

	// only spawn UI on local player controllers
	if (IsLocalPlayerController())
	{
		if (ShouldUseTouchControls())
		{
			// spawn the mobile controls widget
			MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

			if (MobileControlsWidget)
			{
				// add the controls to the player screen
				MobileControlsWidget->AddToPlayerScreen(0);

			} else {

				UE_LOG(LogHive, Error, TEXT("Could not spawn mobile controls widget."));

			}
		}
		

		// spawn the UI widget and add it to the viewport
		VehicleUI = CreateWidget<UHiveUI>(this, VehicleUIClass);

		if (VehicleUI)
		{
			VehicleUI->AddToPlayerScreen(0);
			RefreshPowerUpSlots();
			RefreshPowerUpStatus(true);

		} else {

			UE_LOG(LogHive, Error, TEXT("Could not spawn vehicle UI widget."));

		}
	}
}

void AHivePlayerController::Tick(float Delta)
{
	Super::Tick(Delta);

	if (IsValid(VehiclePawn) && IsValid(VehicleUI))
	{
		VehicleUI->UpdateSpeed(VehiclePawn->GetChaosVehicleMovement()->GetForwardSpeed());
		VehicleUI->UpdateGear(VehiclePawn->GetChaosVehicleMovement()->GetCurrentGear());
		RefreshPowerUpStatus(false);
	}
}

void AHivePlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (VehiclePawn)
	{
		VehiclePawn->OnDestroyed.RemoveDynamic(this, &AHivePlayerController::OnPawnDestroyed);
		UnbindPowerUpSlotEvents(VehiclePawn);
	}

	// get a pointer to the controlled pawn
	VehiclePawn = CastChecked<AHivePawn>(InPawn);

	// subscribe to the pawn's OnDestroyed delegate
	VehiclePawn->OnDestroyed.RemoveDynamic(this, &AHivePlayerController::OnPawnDestroyed);
	VehiclePawn->OnDestroyed.AddDynamic(this, &AHivePlayerController::OnPawnDestroyed);

	BindPowerUpSlotEvents(VehiclePawn);
	RefreshPowerUpSlots();
	RefreshPowerUpStatus(true);
}

void AHivePlayerController::OnPawnDestroyed(AActor* DestroyedPawn)
{
	UnbindPowerUpSlotEvents(Cast<AHivePawn>(DestroyedPawn));

	// find the player start
	TArray<AActor*> ActorList;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), ActorList);

	if (ActorList.Num() > 0)
	{
		// spawn a vehicle at the player start
		const FTransform SpawnTransform = ActorList[0]->GetActorTransform();

		if (AHivePawn* RespawnedVehicle = GetWorld()->SpawnActor<AHivePawn>(VehiclePawnClass, SpawnTransform))
		{
			// possess the vehicle
			Possess(RespawnedVehicle);
		}
	}
}

bool AHivePlayerController::ShouldUseTouchControls() const
{
	// are we on a mobile platform? Should we force touch?
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}

void AHivePlayerController::OnPowerUpSlotChanged(int32 SlotIndex, APowerUpBase* NewContent)
{
	if (!VehicleUI)
	{
		return;
	}

	UPowerUpSlotComponent* PowerUpSlots = GetCurrentPowerUpSlots();
	VehicleUI->UpdatePowerUpSlot(SlotIndex, NewContent, PowerUpSlots ? PowerUpSlots->AreSlotsDisabled() : false);
}

void AHivePlayerController::OnPowerUpSlotsDisabledChanged(bool bSlotsDisabled)
{
	RefreshPowerUpSlots();
	RefreshPowerUpStatus(true);
}

void AHivePlayerController::BindPowerUpSlotEvents(AHivePawn* PawnToBind)
{
	AHiveSportsCar* SportsCar = Cast<AHiveSportsCar>(PawnToBind);
	UPowerUpSlotComponent* PowerUpSlots = SportsCar ? SportsCar->GetPowerUpSlots() : nullptr;
	if (!PowerUpSlots)
	{
		return;
	}

	PowerUpSlots->OnSlotChanged.RemoveDynamic(this, &AHivePlayerController::OnPowerUpSlotChanged);
	PowerUpSlots->OnSlotChanged.AddDynamic(this, &AHivePlayerController::OnPowerUpSlotChanged);
	PowerUpSlots->OnSlotsDisabledChanged.RemoveDynamic(this, &AHivePlayerController::OnPowerUpSlotsDisabledChanged);
	PowerUpSlots->OnSlotsDisabledChanged.AddDynamic(this, &AHivePlayerController::OnPowerUpSlotsDisabledChanged);
}

void AHivePlayerController::UnbindPowerUpSlotEvents(AHivePawn* PawnToUnbind)
{
	AHiveSportsCar* SportsCar = Cast<AHiveSportsCar>(PawnToUnbind);
	UPowerUpSlotComponent* PowerUpSlots = SportsCar ? SportsCar->GetPowerUpSlots() : nullptr;
	if (!PowerUpSlots)
	{
		return;
	}

	PowerUpSlots->OnSlotChanged.RemoveDynamic(this, &AHivePlayerController::OnPowerUpSlotChanged);
	PowerUpSlots->OnSlotsDisabledChanged.RemoveDynamic(this, &AHivePlayerController::OnPowerUpSlotsDisabledChanged);
}

UPowerUpSlotComponent* AHivePlayerController::GetCurrentPowerUpSlots() const
{
	const AHiveSportsCar* SportsCar = Cast<AHiveSportsCar>(VehiclePawn);
	return SportsCar ? SportsCar->GetPowerUpSlots() : nullptr;
}

void AHivePlayerController::RefreshPowerUpSlots()
{
	if (!VehicleUI)
	{
		return;
	}

	UPowerUpSlotComponent* PowerUpSlots = GetCurrentPowerUpSlots();
	if (!PowerUpSlots)
	{
		return;
	}

	const bool bSlotsDisabled = PowerUpSlots->AreSlotsDisabled();
	for (int32 SlotIndex = 0; SlotIndex < PowerUpSlots->GetSlotCount(); ++SlotIndex)
	{
		VehicleUI->UpdatePowerUpSlot(SlotIndex, PowerUpSlots->GetSlotContent(SlotIndex), bSlotsDisabled);
	}
}

void AHivePlayerController::RefreshPowerUpStatus(bool bForceUpdate)
{
	if (!VehicleUI)
	{
		return;
	}

	FHivePowerUpStatusUIData StatusData;

	const AHiveSportsCar* SportsCar = Cast<AHiveSportsCar>(VehiclePawn);
	if (SportsCar)
	{
		if (UPowerUpSlotComponent* PowerUpSlots = SportsCar->GetPowerUpSlots())
		{
			StatusData.bSlotsDisabled = PowerUpSlots->AreSlotsDisabled();
		}

		StatusData.bIsEMPed = SportsCar->bIsEMPed;
		StatusData.bShieldActive = SportsCar->bShieldActive;
		StatusData.bMirrorShieldActive = SportsCar->bMirrorShieldActive;

		if (const UHiveVehicleMovementComponent* HiveMovement = Cast<UHiveVehicleMovementComponent>(SportsCar->GetChaosVehicleMovement()))
		{
			StatusData.bInputFrozen = HiveMovement->IsInputFrozen();
			StatusData.bSpeedCapped = HiveMovement->IsSpeedCapActive();
			StatusData.ActiveSpeedCap = HiveMovement->GetActiveSpeedCap();
			StatusData.bSteeringReduced = HiveMovement->IsTemporarySteeringMultiplierActive();
			StatusData.SteeringMultiplier = HiveMovement->GetTemporarySteeringMultiplier();
		}
	}

	const bool bStatusChanged =
		!bHasLastPowerUpStatus ||
		LastPowerUpStatus.bSlotsDisabled != StatusData.bSlotsDisabled ||
		LastPowerUpStatus.bIsEMPed != StatusData.bIsEMPed ||
		LastPowerUpStatus.bShieldActive != StatusData.bShieldActive ||
		LastPowerUpStatus.bMirrorShieldActive != StatusData.bMirrorShieldActive ||
		LastPowerUpStatus.bInputFrozen != StatusData.bInputFrozen ||
		LastPowerUpStatus.bSpeedCapped != StatusData.bSpeedCapped ||
		LastPowerUpStatus.bSteeringReduced != StatusData.bSteeringReduced ||
		!FMath::IsNearlyEqual(LastPowerUpStatus.ActiveSpeedCap, StatusData.ActiveSpeedCap, 1.0f) ||
		!FMath::IsNearlyEqual(LastPowerUpStatus.SteeringMultiplier, StatusData.SteeringMultiplier, 0.01f);

	if (!bForceUpdate && !bStatusChanged)
	{
		return;
	}

	VehicleUI->UpdatePowerUpStatus(StatusData);
	LastPowerUpStatus = StatusData;
	bHasLastPowerUpStatus = true;
}
