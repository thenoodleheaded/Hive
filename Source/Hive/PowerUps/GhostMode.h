// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PowerUps/PowerUpBase.h"
#include "GhostMode.generated.h"

class AHiveSportsCar;
class UMaterialInterface;
class UNiagaraComponent;
class USkeletalMeshComponent;
class USoundBase;

UCLASS(Blueprintable)
class HIVE_API AGhostMode : public APowerUpBase
{
	GENERATED_BODY()

public:
	AGhostMode();

	UPROPERTY(EditAnywhere, Category = "GhostMode", meta = (ClampMin = "1.0", ClampMax = "10.0", UIMin = "1.0", UIMax = "10.0", ToolTip = "How long the car is invisible and has no collision."))
	float GhostDuration = 4.0f;

	UPROPERTY(EditAnywhere, Category = "GhostMode|Visuals", meta = (ToolTip = "Semi-transparent material applied during ghost. Assign in editor."))
	UMaterialInterface* GhostMaterial = nullptr;

	UPROPERTY(EditAnywhere, Category = "GhostMode|Visuals", meta = (ToolTip = "Particle effect on the car during ghost. Assign in editor."))
	UNiagaraComponent* GhostEffect = nullptr;

	UPROPERTY(EditAnywhere, Category = "GhostMode|Visuals", meta = (ToolTip = "Sound played on activation. Assign in editor."))
	USoundBase* ActivateSound = nullptr;

	UPROPERTY(EditAnywhere, Category = "GhostMode|Visuals", meta = (ToolTip = "Sound played on expire. Assign in editor."))
	USoundBase* ExpireSound = nullptr;

	virtual void OnActivate_Implementation(AHiveSportsCar* InstigatorCar) override;
	virtual void OnExpire_Implementation() override;

protected:
	void StoreOriginalMeshState(USkeletalMeshComponent* CarMesh);
	void ApplyGhostVisuals(USkeletalMeshComponent* CarMesh);
	void RestoreOriginalMeshState(USkeletalMeshComponent* CarMesh);

	TWeakObjectPtr<AHiveSportsCar> GhostedCar;
	ECollisionResponse OriginalPawnResponse = ECR_Block;
	ECollisionResponse OriginalVehicleResponse = ECR_Block;
	bool bOriginalHiddenInGame = false;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInterface>> OriginalMaterials;
};
