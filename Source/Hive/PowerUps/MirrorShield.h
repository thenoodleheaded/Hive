// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PowerUps/PowerUpBase.h"
#include "MirrorShield.generated.h"

class AHiveSportsCar;
class UNiagaraComponent;
class USoundBase;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class HIVE_API AMirrorShield : public APowerUpBase
{
	GENERATED_BODY()

public:
	AMirrorShield();

	UPROPERTY(EditAnywhere, Category = "MirrorShield", meta = (ClampMin = "0.5", ClampMax = "6.0", UIMin = "0.5", UIMax = "6.0", ToolTip = "How long the mirror shield stays active. Shorter than regular shield."))
	float ActiveDuration = 2.5f;

	UPROPERTY(EditAnywhere, Category = "MirrorShield|Visuals", meta = (ToolTip = "Visible bubble around the car. Assign in editor."))
	UStaticMeshComponent* ShieldMesh = nullptr;

	UPROPERTY(EditAnywhere, Category = "MirrorShield|Visuals", meta = (ToolTip = "Particle effect while active. Assign in editor."))
	UNiagaraComponent* ShieldEffect = nullptr;

	UPROPERTY(EditAnywhere, Category = "MirrorShield|Visuals", meta = (ToolTip = "Sound played on activation. Assign in editor."))
	USoundBase* ActivateSound = nullptr;

	UPROPERTY(EditAnywhere, Category = "MirrorShield|Visuals", meta = (ToolTip = "Sound played when a projectile is reflected. Assign in editor."))
	USoundBase* ReflectSound = nullptr;

	virtual void OnActivate_Implementation(AHiveSportsCar* InstigatorCar) override;
	virtual void OnExpire_Implementation() override;

	bool TryReflectProjectile(AActor* Projectile);

protected:
	void ShowShieldVisuals(AHiveSportsCar* TargetCar);
	void HideShieldVisuals();

	TWeakObjectPtr<AHiveSportsCar> ShieldedCar;
	bool bMirrorShieldInstanceActive = false;
};
