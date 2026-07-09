// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PowerUpSlotComponent.generated.h"

class APowerUpBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSlotChanged, int32, SlotIndex, APowerUpBase*, NewContent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSlotsDisabledChanged, bool, bSlotsDisabled);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class HIVE_API UPowerUpSlotComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPowerUpSlotComponent();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PowerUps")
	int32 MaxSlots = 2;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PowerUps")
	TArray<APowerUpBase*> CurrentSlots;

	UPROPERTY(BlueprintAssignable, Category = "PowerUps")
	FOnSlotChanged OnSlotChanged;

	UPROPERTY(BlueprintAssignable, Category = "PowerUps")
	FOnSlotsDisabledChanged OnSlotsDisabledChanged;

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "PowerUps")
	void SetSlotsDisabled(bool bInSlotsDisabled);

	UFUNCTION(BlueprintPure, Category = "PowerUps")
	bool AreSlotsDisabled() const { return bSlotsDisabled; }

	UFUNCTION(BlueprintPure, Category = "PowerUps")
	int32 GetSlotCount() const { return CurrentSlots.Num(); }

	UFUNCTION(BlueprintCallable, Category = "PowerUps")
	bool TryAddPowerUp(APowerUpBase* PowerUp);

	UFUNCTION(BlueprintCallable, Category = "PowerUps")
	void ActivateSlot(int32 SlotIndex);

	UFUNCTION(BlueprintCallable, Category = "PowerUps")
	void ClearSlot(int32 SlotIndex);

	UFUNCTION(BlueprintCallable, Category = "PowerUps")
	APowerUpBase* GetSlotContent(int32 SlotIndex) const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PowerUps")
	bool bSlotsDisabled = false;
};
