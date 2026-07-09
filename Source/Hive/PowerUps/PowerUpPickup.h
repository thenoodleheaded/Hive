// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PowerUpPickup.generated.h"

class APowerUpBase;
class UPointLightComponent;
class URotatingMovementComponent;
class USphereComponent;
class UStaticMeshComponent;
class UNiagaraComponent;

UCLASS(Blueprintable)
class HIVE_API APowerUpPickup : public AActor
{
	GENERATED_BODY()

public:
	APowerUpPickup();

	UPROPERTY(EditAnywhere, Category = "Pickup", meta = (ToolTip = "Power-up class this pickup gives. Null means inactive."))
	TSubclassOf<APowerUpBase> PowerUpClass;

	UPROPERTY(EditAnywhere, Category = "Pickup", meta = (ClampMin = "1.0", ClampMax = "60.0", UIMin = "1.0", UIMax = "60.0", ToolTip = "Seconds before this pickup reappears after being collected."))
	float RespawnTime = 8.0f;

	UPROPERTY(EditAnywhere, Category = "Pickup", meta = (ToolTip = "If true, pickup reappears after RespawnTime."))
	bool bAutoRespawn = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pickup|Visuals")
	UStaticMeshComponent* PickupMesh = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pickup|Visuals")
	URotatingMovementComponent* RotatingMovement = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pickup|Visuals")
	USphereComponent* CollisionSphere = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pickup|Visuals")
	UPointLightComponent* PickupLight = nullptr;

	UPROPERTY(EditAnywhere, Category = "Pickup|Visuals", meta = (ToolTip = "Light color for this pickup."))
	FLinearColor PickupLightColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, Category = "Pickup|Visuals", meta = (ToolTip = "Optional respawn burst effect. Assign in editor."))
	UNiagaraComponent* RespawnEffect = nullptr;

	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

	UFUNCTION()
	void Respawn();

protected:
	UFUNCTION()
	void OnPickupOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	void SetPickupActive(bool bIsActive);

	FTimerHandle RespawnTimerHandle;
};
