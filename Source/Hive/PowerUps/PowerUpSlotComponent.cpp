// Copyright Epic Games, Inc. All Rights Reserved.

#include "PowerUps/PowerUpSlotComponent.h"
#include "Hive.h"
#include "HivePawn.h"
#include "HiveSportsCar.h"
#include "HiveVehicleMovementComponent.h"
#include "PowerUps/PowerUpBase.h"
#include "UCarPhysicsProfile.h"

UPowerUpSlotComponent::UPowerUpSlotComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	CurrentSlots.SetNum(MaxSlots);
}

void UPowerUpSlotComponent::BeginPlay()
{
	Super::BeginPlay();

	MaxSlots = 2;

	if (const AHiveSportsCar* SportsCar = Cast<AHiveSportsCar>(GetOwner()))
	{
		const UCarPhysicsProfile* PhysicsProfile = nullptr;
		if (const UHiveVehicleMovementComponent* HiveMovement = Cast<UHiveVehicleMovementComponent>(SportsCar->GetChaosVehicleMovement()))
		{
			PhysicsProfile = HiveMovement->PhysicsProfile;
		}

		if (!PhysicsProfile)
		{
			PhysicsProfile = SportsCar->GetPhysicsProfile();
		}

		if (PhysicsProfile)
		{
			MaxSlots = FMath::Max(0, PhysicsProfile->SlotCount);
		}
	}

	CurrentSlots.SetNum(MaxSlots);
}

void UPowerUpSlotComponent::SetSlotsDisabled(bool bInSlotsDisabled)
{
	if (bSlotsDisabled == bInSlotsDisabled)
	{
		return;
	}

	bSlotsDisabled = bInSlotsDisabled;
	OnSlotsDisabledChanged.Broadcast(bSlotsDisabled);
}

bool UPowerUpSlotComponent::TryAddPowerUp(APowerUpBase* PowerUp)
{
	if (!IsValid(PowerUp))
	{
		return false;
	}

	for (int32 SlotIndex = 0; SlotIndex < CurrentSlots.Num(); ++SlotIndex)
	{
		if (!IsValid(CurrentSlots[SlotIndex]))
		{
			CurrentSlots[SlotIndex] = PowerUp;

			if (AHiveSportsCar* SportsCar = Cast<AHiveSportsCar>(GetOwner()))
			{
				PowerUp->OnPickup(SportsCar);
			}

			OnSlotChanged.Broadcast(SlotIndex, PowerUp);
			return true;
		}
	}

	return false;
}

void UPowerUpSlotComponent::ActivateSlot(int32 SlotIndex)
{
	if (bSlotsDisabled)
	{
		return;
	}

	if (!CurrentSlots.IsValidIndex(SlotIndex) || !IsValid(CurrentSlots[SlotIndex]))
	{
		return;
	}

	APowerUpBase* ActivatedPowerUp = CurrentSlots[SlotIndex];
	const TSubclassOf<APowerUpBase> ActivatedPowerUpClass = ActivatedPowerUp->GetClass();
	CurrentSlots[SlotIndex]->OnActivate(Cast<AHiveSportsCar>(GetOwner()));
	ClearSlot(SlotIndex);
	UE_LOG(LogHive, Verbose, TEXT("Power-up slot %d cleared after activating %s."), SlotIndex, *GetNameSafe(ActivatedPowerUp));

#if WITH_EDITOR
	const AHivePawn* HivePawnOwner = Cast<AHivePawn>(GetOwner());
	if (HivePawnOwner && HivePawnOwner->ShouldDebugInfinitePowerUps() && ActivatedPowerUpClass)
	{
		UWorld* World = GetWorld();
		AActor* OwnerActor = GetOwner();
		if (!World || !OwnerActor || !CurrentSlots.IsValidIndex(SlotIndex) || IsValid(CurrentSlots[SlotIndex]))
		{
			return;
		}

		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Owner = OwnerActor;

		APowerUpBase* ReplacementPowerUp = World->SpawnActor<APowerUpBase>(ActivatedPowerUpClass, OwnerActor->GetActorLocation(), OwnerActor->GetActorRotation(), SpawnParameters);
		if (!ReplacementPowerUp)
		{
			UE_LOG(LogHive, Warning, TEXT("[DebugPowerUps] Failed to refill slot %d with %s."), SlotIndex, *GetNameSafe(ActivatedPowerUpClass.Get()));
			return;
		}

		CurrentSlots[SlotIndex] = ReplacementPowerUp;

		if (AHiveSportsCar* SportsCar = Cast<AHiveSportsCar>(OwnerActor))
		{
			ReplacementPowerUp->OnPickup(SportsCar);
		}

		OnSlotChanged.Broadcast(SlotIndex, ReplacementPowerUp);
		UE_LOG(LogHive, Verbose, TEXT("[DebugPowerUps] Refilled slot %d with %s because bDebugInfinitePowerUps is enabled."), SlotIndex, *GetNameSafe(ReplacementPowerUp));
	}
#endif
}

void UPowerUpSlotComponent::ClearSlot(int32 SlotIndex)
{
	if (!CurrentSlots.IsValidIndex(SlotIndex))
	{
		return;
	}

	CurrentSlots[SlotIndex] = nullptr;
	OnSlotChanged.Broadcast(SlotIndex, nullptr);
}

APowerUpBase* UPowerUpSlotComponent::GetSlotContent(int32 SlotIndex) const
{
	return CurrentSlots.IsValidIndex(SlotIndex) ? CurrentSlots[SlotIndex] : nullptr;
}
