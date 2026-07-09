// Copyright Epic Games, Inc. All Rights Reserved.


#include "HiveUI.h"
#include "PowerUps/PowerUpBase.h"

void UHiveUI::UpdateSpeed(float NewSpeed)
{
	// format the speed to KPH or MPH
	float FormattedSpeed = FMath::Abs(NewSpeed) * (bIsMPH ? 0.022f : 0.036f);

	// call the Blueprint handler
	OnSpeedUpdate(FormattedSpeed);
}

void UHiveUI::UpdateGear(int32 NewGear)
{
	// call the Blueprint handler
	OnGearUpdate(NewGear);
}

void UHiveUI::UpdatePowerUpSlot(int32 SlotIndex, APowerUpBase* PowerUp, bool bSlotsDisabled)
{
	FHivePowerUpSlotUIData SlotData;
	SlotData.SlotIndex = SlotIndex;
	SlotData.PowerUp = PowerUp;
	SlotData.bSlotOccupied = IsValid(PowerUp);
	SlotData.DisplayName = SlotData.bSlotOccupied ? PowerUp->DisplayName : FText::GetEmpty();
	SlotData.HUDIcon = SlotData.bSlotOccupied ? PowerUp->HUDIcon : nullptr;
	SlotData.bSlotsDisabled = bSlotsDisabled;

	OnPowerUpSlotUpdate(SlotData);
}

void UHiveUI::UpdatePowerUpStatus(const FHivePowerUpStatusUIData& StatusData)
{
	OnPowerUpStatusUpdate(StatusData);
}
