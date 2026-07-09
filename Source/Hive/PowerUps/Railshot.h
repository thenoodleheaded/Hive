// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PowerUps/PowerUpBase.h"
#include "Railshot.generated.h"

class AHiveSportsCar;
class UNiagaraComponent;
class UProjectileMovementComponent;
class USoundBase;
class USphereComponent;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class HIVE_API ARailshotProjectile : public AActor
{
	GENERATED_BODY()

public:
	ARailshotProjectile();

	UPROPERTY(EditAnywhere, Category = "Railshot", meta = (ClampMin = "1000.0", ClampMax = "15000.0", UIMin = "1000.0", UIMax = "15000.0", ToolTip = "Straight-line projectile speed in cm/s."))
	float ProjectileSpeed = 9000.0f;

	UPROPERTY(EditAnywhere, Category = "Railshot", meta = (ClampMin = "1.0", ClampMax = "10.0", UIMin = "1.0", UIMax = "10.0", ToolTip = "Seconds before the railshot disappears."))
	float ProjectileLifetime = 3.0f;

	UPROPERTY(EditAnywhere, Category = "Railshot", meta = (ClampMin = "10.0", ClampMax = "100.0", UIMin = "10.0", UIMax = "100.0", ToolTip = "Collision radius of the railshot."))
	float ProjectileRadius = 25.0f;

	UPROPERTY(EditAnywhere, Category = "Railshot", meta = (ClampMin = "50.0", ClampMax = "600.0", UIMin = "50.0", UIMax = "600.0", ToolTip = "Blast radius on impact."))
	float ExplosionRadius = 220.0f;

	UPROPERTY(EditAnywhere, Category = "Railshot", meta = (ClampMin = "0.0", ClampMax = "10000.0", UIMin = "0.0", UIMax = "10000.0", ToolTip = "Sideways drift kick impulse applied at the rear."))
	float ImpactImpulse = 2000.0f;

	UPROPERTY(EditAnywhere, Category = "Railshot", meta = (ClampMin = "0.0", ClampMax = "400.0", UIMin = "0.0", UIMax = "400.0", ToolTip = "Rearward offset where drift kick is applied."))
	float DriftKickRearOffset = 150.0f;

	UPROPERTY(EditAnywhere, Category = "Railshot", meta = (ClampMin = "0.0", ClampMax = "10000.0", UIMin = "0.0", UIMax = "10000.0", ToolTip = "Downward impulse that keeps hits grounded."))
	float DriftKickDownforceImpulse = 3500.0f;

	UPROPERTY(EditAnywhere, Category = "Railshot", meta = (ClampMin = "0.0", ClampMax = "2000.0", UIMin = "0.0", UIMax = "2000.0", ToolTip = "Maximum upward velocity allowed after hit."))
	float MaxUpwardVelocityAfterHit = 250.0f;

	UPROPERTY(EditAnywhere, Category = "Railshot", meta = (ClampMin = "0.1", ClampMax = "5.0", UIMin = "0.5", UIMax = "5.0", ToolTip = "Seconds of reduced steering after a direct hit."))
	float SteeringReductionDuration = 1.5f;

	UPROPERTY(EditAnywhere, Category = "Railshot|Visuals", meta = (ToolTip = "Visible railshot projectile."))
	UStaticMeshComponent* ProjectileMesh = nullptr;

	UPROPERTY(EditAnywhere, Category = "Railshot|Visuals", meta = (ToolTip = "Particle trail on the railshot. Assign in editor."))
	UNiagaraComponent* TrailEffect = nullptr;

	UPROPERTY(EditAnywhere, Category = "Railshot|Visuals", meta = (ToolTip = "Explosion particle effect. Assign in editor."))
	UNiagaraComponent* ExplosionEffect = nullptr;

	UPROPERTY(EditAnywhere, Category = "Railshot|Visuals", meta = (ToolTip = "Sound played on impact. Assign in editor."))
	USoundBase* ImpactSound = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Railshot")
	USphereComponent* CollisionSphere = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Railshot")
	UProjectileMovementComponent* ProjectileMovement = nullptr;

	void Initialize(AHiveSportsCar* InSourceCar, float InProjectileSpeed, float InProjectileLifetime, float InProjectileRadius, float InExplosionRadius, float InImpactImpulse, float InDriftKickRearOffset, float InDriftKickDownforceImpulse, float InMaxUpwardVelocityAfterHit, float InSteeringReductionDuration, const FVector& FireDirection);

	virtual void BeginPlay() override;

protected:
	UFUNCTION()
	void OnRailshotHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	void Explode(const FVector& HitLocation, AHiveSportsCar* DirectHitCar);
	void PlayImpactEffects(const FVector& HitLocation);
	void FinishProjectileLifetime();
	void HideProjectileAfterImpact();
	void EnableHitRegistration();

	TWeakObjectPtr<AHiveSportsCar> SourceCar;
	FTimerHandle LifetimeTimerHandle;
	FTimerHandle HitGraceTimerHandle;
	bool bCanRegisterHit = false;
};

UCLASS(Blueprintable)
class HIVE_API ARailshot : public APowerUpBase
{
	GENERATED_BODY()

public:
	ARailshot();

	UPROPERTY(EditAnywhere, Category = "Railshot", meta = (ClampMin = "1000.0", ClampMax = "15000.0", UIMin = "1000.0", UIMax = "15000.0", ToolTip = "Straight-line projectile speed in cm/s."))
	float ProjectileSpeed = 9000.0f;

	UPROPERTY(EditAnywhere, Category = "Railshot", meta = (ClampMin = "1.0", ClampMax = "10.0", UIMin = "1.0", UIMax = "10.0", ToolTip = "Seconds before the railshot disappears."))
	float ProjectileLifetime = 3.0f;

	UPROPERTY(EditAnywhere, Category = "Railshot", meta = (ClampMin = "10.0", ClampMax = "100.0", UIMin = "10.0", UIMax = "100.0", ToolTip = "Collision radius of the railshot."))
	float ProjectileRadius = 25.0f;

	UPROPERTY(EditAnywhere, Category = "Railshot", meta = (ClampMin = "50.0", ClampMax = "600.0", UIMin = "50.0", UIMax = "600.0", ToolTip = "Blast radius on impact."))
	float ExplosionRadius = 220.0f;

	UPROPERTY(EditAnywhere, Category = "Railshot", meta = (ClampMin = "0.0", ClampMax = "10000.0", UIMin = "0.0", UIMax = "10000.0", ToolTip = "Sideways drift kick impulse applied at the rear."))
	float ImpactImpulse = 2000.0f;

	UPROPERTY(EditAnywhere, Category = "Railshot", meta = (ClampMin = "0.0", ClampMax = "400.0", UIMin = "0.0", UIMax = "400.0", ToolTip = "Rearward offset where drift kick is applied."))
	float DriftKickRearOffset = 150.0f;

	UPROPERTY(EditAnywhere, Category = "Railshot", meta = (ClampMin = "0.0", ClampMax = "10000.0", UIMin = "0.0", UIMax = "10000.0", ToolTip = "Downward impulse that keeps hits grounded."))
	float DriftKickDownforceImpulse = 3500.0f;

	UPROPERTY(EditAnywhere, Category = "Railshot", meta = (ClampMin = "0.0", ClampMax = "2000.0", UIMin = "0.0", UIMax = "2000.0", ToolTip = "Maximum upward velocity allowed after hit."))
	float MaxUpwardVelocityAfterHit = 250.0f;

	UPROPERTY(EditAnywhere, Category = "Railshot", meta = (ClampMin = "0.1", ClampMax = "5.0", UIMin = "0.5", UIMax = "5.0", ToolTip = "Seconds of reduced steering after a direct hit."))
	float SteeringReductionDuration = 1.5f;

	UPROPERTY(EditAnywhere, Category = "Railshot|Visuals", meta = (ToolTip = "Visible railshot projectile."))
	UStaticMeshComponent* ProjectileMesh = nullptr;

	UPROPERTY(EditAnywhere, Category = "Railshot|Visuals", meta = (ToolTip = "Particle trail on the railshot. Assign in editor."))
	UNiagaraComponent* TrailEffect = nullptr;

	UPROPERTY(EditAnywhere, Category = "Railshot|Visuals", meta = (ToolTip = "Explosion particle effect. Assign in editor."))
	UNiagaraComponent* ExplosionEffect = nullptr;

	UPROPERTY(EditAnywhere, Category = "Railshot|Visuals", meta = (ToolTip = "Sound played on firing. Assign in editor."))
	USoundBase* FireSound = nullptr;

	UPROPERTY(EditAnywhere, Category = "Railshot|Visuals", meta = (ToolTip = "Sound played on impact. Assign in editor."))
	USoundBase* ImpactSound = nullptr;

	virtual void OnActivate_Implementation(AHiveSportsCar* InstigatorCar) override;
};
