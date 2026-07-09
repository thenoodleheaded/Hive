// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PowerUps/PowerUpBase.h"
#include "EMPBurst.generated.h"

class AHiveSportsCar;
class UMaterialInterface;
class UNiagaraComponent;
class USoundBase;

struct FEMPAffectedCarState
{
	TArray<UMaterialInterface*> OriginalMaterials;
	FTimerHandle RestoreTimerHandle;
};

UCLASS(Blueprintable)
class HIVE_API AEMPBurst : public APowerUpBase
{
	GENERATED_BODY()

public:
	AEMPBurst();

	UPROPERTY(EditAnywhere, Category = "EMPBurst", meta = (ClampMin = "200.0", ClampMax = "2000.0", UIMin = "200.0", UIMax = "2000.0", ToolTip = "Radius of the EMP blast in cm."))
	float BlastRadius = 800.0f;

	UPROPERTY(EditAnywhere, Category = "EMPBurst", meta = (ClampMin = "1.0", ClampMax = "10.0", UIMin = "1.0", UIMax = "10.0", ToolTip = "How long affected cars have systems disabled."))
	float DisableDuration = 4.0f;

	UPROPERTY(EditAnywhere, Category = "EMPBurst", meta = (ToolTip = "If true, EMP also affects the car that fired it."))
	bool bAffectSelf = false;

	UPROPERTY(EditAnywhere, Category = "EMPBurst|Visuals", meta = (ToolTip = "Expanding ring particle effect on activation. Assign in editor."))
	UNiagaraComponent* BlastEffect = nullptr;

	UPROPERTY(EditAnywhere, Category = "EMPBurst|Visuals", meta = (ToolTip = "Sound played on activation. Assign in editor."))
	USoundBase* BlastSound = nullptr;

	UPROPERTY(EditAnywhere, Category = "EMPBurst|Visuals", meta = (ToolTip = "Material briefly applied to hit cars. Assign in editor."))
	UMaterialInterface* AffectedCarMaterial = nullptr;

	virtual void OnActivate_Implementation(AHiveSportsCar* InstigatorCar) override;
	virtual void Destroyed() override;

protected:
	void ApplyEMPToCar(AHiveSportsCar* SportsCar);
	void RestoreCar(AHiveSportsCar* SportsCar);
	void PlayBlastVisuals(const FVector& BlastLocation);

	TMap<TWeakObjectPtr<AHiveSportsCar>, FEMPAffectedCarState> AffectedCars;
};
