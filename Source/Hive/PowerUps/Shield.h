// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PowerUps/PowerUpBase.h"
#include "Shield.generated.h"

class AHiveSportsCar;
class UNiagaraComponent;
class USoundBase;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class HIVE_API AShield : public APowerUpBase
{
	GENERATED_BODY()

public:
	AShield();

	UPROPERTY(EditAnywhere, Category = "Shield", meta = (ClampMin = "1.0", ClampMax = "15.0", UIMin = "1.0", UIMax = "15.0", ToolTip = "How long the shield stays active before expiring."))
	float ShieldDuration = 6.0f;

	UPROPERTY(EditAnywhere, Category = "Shield", meta = (ClampMin = "1", ClampMax = "10", UIMin = "1", UIMax = "10", ToolTip = "How many hits the shield absorbs before breaking early."))
	int32 MaxHitsAbsorbed = 3;

	UPROPERTY(EditAnywhere, Category = "Shield|Visuals", meta = (ToolTip = "Visible bubble around the car. Assign in editor."))
	UStaticMeshComponent* ShieldMesh = nullptr;

	UPROPERTY(EditAnywhere, Category = "Shield|Visuals", meta = (ToolTip = "Particle effect while active. Assign in editor."))
	UNiagaraComponent* ShieldEffect = nullptr;

	UPROPERTY(EditAnywhere, Category = "Shield|Visuals", meta = (ToolTip = "Sound played on activation. Assign in editor."))
	USoundBase* ActivateSound = nullptr;

	UPROPERTY(EditAnywhere, Category = "Shield|Visuals", meta = (ToolTip = "Sound played when a hit is absorbed. Assign in editor."))
	USoundBase* HitAbsorbedSound = nullptr;

	UPROPERTY(EditAnywhere, Category = "Shield|Visuals", meta = (ToolTip = "Sound played when the shield breaks. Assign in editor."))
	USoundBase* ShieldBrokenSound = nullptr;

	virtual void OnActivate_Implementation(AHiveSportsCar* InstigatorCar) override;
	virtual void OnExpire_Implementation() override;

	bool TryAbsorbHit();
	void BreakShield(bool bPlayBrokenSound = true);

protected:
	void ShowShieldVisuals(AHiveSportsCar* TargetCar);
	void HideShieldVisuals();

	TWeakObjectPtr<AHiveSportsCar> ShieldedCar;
	int32 HitsRemaining = 0;
	bool bShieldInstanceActive = false;
};
