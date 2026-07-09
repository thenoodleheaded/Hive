// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HivePawn.h"
#include "HiveSportsCar.generated.h"

class UCarPhysicsProfile;
class UPowerUpSlotComponent;
class USphereComponent;
class AActor;
class AMirrorShield;
class AShield;

/**
 *  Sports car wheeled vehicle implementation
 */
UCLASS(abstract)
class AHiveSportsCar : public AHivePawn
{
	GENERATED_BODY()

protected:

	UPROPERTY(EditAnywhere, Category = "Profile")
	UCarPhysicsProfile* PhysicsProfile = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PowerUps")
	UPowerUpSlotComponent* PowerUpSlots = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gameplay")
	USphereComponent* GameplayTriggerVolume = nullptr;
		
public:

	AHiveSportsCar(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PowerUps")
	bool bIsEMPed = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PowerUps")
	bool bShieldActive = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PowerUps")
	bool bMirrorShieldActive = false;

	UFUNCTION(BlueprintPure, Category = "Profile")
	UCarPhysicsProfile* GetPhysicsProfile() const { return PhysicsProfile; }

	UFUNCTION(BlueprintPure, Category = "PowerUps")
	UPowerUpSlotComponent* GetPowerUpSlots() const { return PowerUpSlots; }

	UFUNCTION(BlueprintCallable, Category = "PowerUps")
	void ActivatePowerUp(int32 SlotIndex);

	bool TryAbsorbHit();
	bool TryReflectProjectile(AActor* Projectile);
	void SetActiveShield(AShield* Shield);
	void ClearActiveShield(AShield* Shield);
	void SetActiveMirrorShield(AMirrorShield* MirrorShield);
	void ClearActiveMirrorShield(AMirrorShield* MirrorShield);

private:

	void SyncPhysicsProfileToMovementComponent() const;

	UPROPERTY(Transient)
	TObjectPtr<AShield> ActiveShield = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<AMirrorShield> ActiveMirrorShield = nullptr;
};
