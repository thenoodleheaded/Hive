// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HiveUI.generated.h"

class APowerUpBase;
class UTexture2D;

USTRUCT(BlueprintType)
struct HIVE_API FHivePowerUpSlotUIData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "PowerUps")
	int32 SlotIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "PowerUps")
	APowerUpBase* PowerUp = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "PowerUps")
	FText DisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "PowerUps")
	UTexture2D* HUDIcon = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "PowerUps")
	bool bSlotOccupied = false;

	UPROPERTY(BlueprintReadOnly, Category = "PowerUps")
	bool bSlotsDisabled = false;
};

USTRUCT(BlueprintType)
struct HIVE_API FHivePowerUpStatusUIData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "PowerUps")
	bool bSlotsDisabled = false;

	UPROPERTY(BlueprintReadOnly, Category = "PowerUps")
	bool bIsEMPed = false;

	UPROPERTY(BlueprintReadOnly, Category = "PowerUps")
	bool bShieldActive = false;

	UPROPERTY(BlueprintReadOnly, Category = "PowerUps")
	bool bMirrorShieldActive = false;

	UPROPERTY(BlueprintReadOnly, Category = "PowerUps")
	bool bInputFrozen = false;

	UPROPERTY(BlueprintReadOnly, Category = "PowerUps")
	bool bSpeedCapped = false;

	UPROPERTY(BlueprintReadOnly, Category = "PowerUps")
	float ActiveSpeedCap = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "PowerUps")
	bool bSteeringReduced = false;

	UPROPERTY(BlueprintReadOnly, Category = "PowerUps")
	float SteeringMultiplier = 1.0f;
};

/**
 *  Simple Vehicle HUD class
 *  Displays the current speed and gear.
 *  Widget setup is handled in a Blueprint subclass.
 */
UCLASS(abstract)
class UHiveUI : public UUserWidget
{
	GENERATED_BODY()
	
protected:

	/** Controls the display of speed in Km/h or MPH */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Vehicle")
	bool bIsMPH = false;

public:

	/** Called to update the speed display */
	void UpdateSpeed(float NewSpeed);

	/** Called to update the gear display */
	void UpdateGear(int32 NewGear);

	/** Called when a power-up slot changes or when the current vehicle is refreshed */
	void UpdatePowerUpSlot(int32 SlotIndex, APowerUpBase* PowerUp, bool bSlotsDisabled);

	/** Called to update power-up related status flags */
	void UpdatePowerUpStatus(const FHivePowerUpStatusUIData& StatusData);

protected:

	/** Implemented in Blueprint to display the new speed */
	UFUNCTION(BlueprintImplementableEvent, Category="Vehicle")
	void OnSpeedUpdate(float NewSpeed);

	/** Implemented in Blueprint to display the new gear */
	UFUNCTION(BlueprintImplementableEvent, Category="Vehicle")
	void OnGearUpdate(int32 NewGear);

	/** Implemented in Blueprint to display one power-up slot */
	UFUNCTION(BlueprintImplementableEvent, Category="PowerUps")
	void OnPowerUpSlotUpdate(const FHivePowerUpSlotUIData& SlotData);

	/** Implemented in Blueprint to display active power-up related states */
	UFUNCTION(BlueprintImplementableEvent, Category="PowerUps")
	void OnPowerUpStatusUpdate(const FHivePowerUpStatusUIData& StatusData);
};
