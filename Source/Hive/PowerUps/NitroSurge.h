// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PowerUps/PowerUpBase.h"
#include "NitroSurge.generated.h"

class UMaterialInterface;
class UNiagaraComponent;
class USoundBase;

UCLASS(Blueprintable)
class HIVE_API ANitroSurge : public APowerUpBase
{
	GENERATED_BODY()

public:
	ANitroSurge();

	UPROPERTY(EditAnywhere, Category = "NitroSurge", meta = (ClampMin = "500.0", ClampMax = "8000.0", UIMin = "500.0", UIMax = "8000.0", ToolTip = "Instant velocity impulse applied to the car on activation."))
	float ImpulseStrength = 3000.0f;

	UPROPERTY(EditAnywhere, Category = "NitroSurge|Visuals", meta = (ToolTip = "Particle effect played on the car during activation. Assign in editor."))
	UNiagaraComponent* NitroEffect = nullptr;

	UPROPERTY(EditAnywhere, Category = "NitroSurge|Visuals", meta = (ToolTip = "Sound played on activation. Assign in editor."))
	USoundBase* NitroSound = nullptr;

	UPROPERTY(EditAnywhere, Category = "NitroSurge|Visuals", meta = (ToolTip = "Material briefly applied to car mesh on activation. Assign in editor."))
	UMaterialInterface* NitroFlashMaterial = nullptr;

	virtual void OnPickup_Implementation(AHiveSportsCar* Carrier) override;
	virtual void OnActivate_Implementation(AHiveSportsCar* InstigatorCar) override;
	virtual void OnExpire_Implementation() override;

protected:
	UFUNCTION()
	void RestoreNitroFlashMaterial();

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInterface>> OriginalMaterials;

	FTimerHandle NitroFlashTimerHandle;
};
