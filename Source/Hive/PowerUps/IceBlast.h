// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PowerUps/PowerUpBase.h"
#include "IceBlast.generated.h"

class AHiveSportsCar;
class UNiagaraComponent;
class UProjectileMovementComponent;
class USoundBase;
class USphereComponent;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class HIVE_API AIceBlastProjectile : public AActor
{
	GENERATED_BODY()

public:
	AIceBlastProjectile();

	UPROPERTY(EditAnywhere, Category = "IceBlast", meta = (ClampMin = "1000.0", ClampMax = "10000.0", UIMin = "1000.0", UIMax = "10000.0", ToolTip = "Speed of the ice projectile in cm/s."))
	float ProjectileSpeed = 4000.0f;

	UPROPERTY(EditAnywhere, Category = "IceBlast", meta = (ClampMin = "0.5", ClampMax = "8.0", UIMin = "0.5", UIMax = "8.0", ToolTip = "How long the hit car's direction is locked."))
	float FreezeDuration = 2.5f;

	UPROPERTY(EditAnywhere, Category = "IceBlast", meta = (ClampMin = "1.0", ClampMax = "10.0", UIMin = "1.0", UIMax = "10.0", ToolTip = "Seconds before the projectile disappears if it hits nothing."))
	float ProjectileLifetime = 4.0f;

	UPROPERTY(EditAnywhere, Category = "IceBlast", meta = (ClampMin = "10.0", ClampMax = "100.0", UIMin = "10.0", UIMax = "100.0", ToolTip = "Collision sphere radius of the projectile."))
	float ProjectileRadius = 30.0f;

	UPROPERTY(EditAnywhere, Category = "IceBlast", meta = (ClampMin = "0.0", ClampMax = "200.0", UIMin = "0.0", UIMax = "200.0", ToolTip = "Height above the car used when spawning the projectile."))
	float SpawnHeightOffset = 80.0f;

	UPROPERTY(EditAnywhere, Category = "IceBlast|Visuals", meta = (ToolTip = "Visible mesh on the projectile."))
	UStaticMeshComponent* ProjectileMesh = nullptr;

	UPROPERTY(EditAnywhere, Category = "IceBlast|Visuals", meta = (ToolTip = "Particle trail on the projectile. Assign in editor."))
	UNiagaraComponent* TrailEffect = nullptr;

	UPROPERTY(EditAnywhere, Category = "IceBlast|Visuals", meta = (ToolTip = "Particle burst on impact. Assign in editor."))
	UNiagaraComponent* HitEffect = nullptr;

	UPROPERTY(EditAnywhere, Category = "IceBlast|Visuals", meta = (ToolTip = "Sound played on impact. Assign in editor."))
	USoundBase* HitSound = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "IceBlast")
	USphereComponent* CollisionSphere = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "IceBlast")
	UProjectileMovementComponent* ProjectileMovement = nullptr;

	void Initialize(AHiveSportsCar* InSourceCar, float InProjectileSpeed, float InFreezeDuration, float InProjectileLifetime, float InProjectileRadius, const FVector& FireDirection);

	virtual void BeginPlay() override;

protected:
	UFUNCTION()
	void OnProjectileHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	void ApplyFullInputFreeze();
	void EndInputFreeze();
	void FinishProjectileLifetime();
	void EnableProjectileHitNotifications();
	void PlayHitEffects(const FVector& HitLocation);
	void HideProjectileAfterHit();

	TWeakObjectPtr<AHiveSportsCar> SourceCar;
	TWeakObjectPtr<AHiveSportsCar> FrozenCar;
	FTimerHandle FreezeEndTimerHandle;
	FTimerHandle LifetimeTimerHandle;
	FTimerHandle HitNotificationTimerHandle;
};

UCLASS(Blueprintable)
class HIVE_API AIceBlast : public APowerUpBase
{
	GENERATED_BODY()

public:
	AIceBlast();

	UPROPERTY(EditAnywhere, Category = "IceBlast", meta = (ClampMin = "1000.0", ClampMax = "10000.0", UIMin = "1000.0", UIMax = "10000.0", ToolTip = "Speed of the ice projectile in cm/s."))
	float ProjectileSpeed = 4000.0f;

	UPROPERTY(EditAnywhere, Category = "IceBlast", meta = (ClampMin = "0.5", ClampMax = "8.0", UIMin = "0.5", UIMax = "8.0", ToolTip = "How long the hit car's direction is locked."))
	float FreezeDuration = 2.5f;

	UPROPERTY(EditAnywhere, Category = "IceBlast", meta = (ClampMin = "1.0", ClampMax = "10.0", UIMin = "1.0", UIMax = "10.0", ToolTip = "Seconds before the projectile disappears if it hits nothing."))
	float ProjectileLifetime = 4.0f;

	UPROPERTY(EditAnywhere, Category = "IceBlast", meta = (ClampMin = "10.0", ClampMax = "100.0", UIMin = "10.0", UIMax = "100.0", ToolTip = "Collision sphere radius of the projectile."))
	float ProjectileRadius = 30.0f;

	UPROPERTY(EditAnywhere, Category = "IceBlast", meta = (ClampMin = "0.0", ClampMax = "200.0", UIMin = "0.0", UIMax = "200.0", ToolTip = "Height above the car used when spawning the projectile."))
	float SpawnHeightOffset = 80.0f;

	UPROPERTY(EditAnywhere, Category = "IceBlast|Visuals", meta = (ToolTip = "Visible mesh on the projectile."))
	UStaticMeshComponent* ProjectileMesh = nullptr;

	UPROPERTY(EditAnywhere, Category = "IceBlast|Visuals", meta = (ToolTip = "Particle trail on the projectile. Assign in editor."))
	UNiagaraComponent* TrailEffect = nullptr;

	UPROPERTY(EditAnywhere, Category = "IceBlast|Visuals", meta = (ToolTip = "Particle burst on impact. Assign in editor."))
	UNiagaraComponent* HitEffect = nullptr;

	UPROPERTY(EditAnywhere, Category = "IceBlast|Visuals", meta = (ToolTip = "Sound played when firing. Assign in editor."))
	USoundBase* FireSound = nullptr;

	UPROPERTY(EditAnywhere, Category = "IceBlast|Visuals", meta = (ToolTip = "Sound played on impact. Assign in editor."))
	USoundBase* HitSound = nullptr;

	virtual void OnActivate_Implementation(AHiveSportsCar* InstigatorCar) override;
};
