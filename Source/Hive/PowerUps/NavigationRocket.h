// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PowerUps/PowerUpBase.h"
#include "NavigationRocket.generated.h"

class AHiveSportsCar;
class UNiagaraComponent;
class UProjectileMovementComponent;
class USoundBase;
class USphereComponent;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class HIVE_API ANavigationRocketProjectile : public AActor
{
	GENERATED_BODY()

public:
	ANavigationRocketProjectile();

	UPROPERTY(EditAnywhere, Category = "NavigationRocket", meta = (ClampMin = "500.0", ClampMax = "6000.0", UIMin = "500.0", UIMax = "6000.0", ToolTip = "Base travel speed of the rocket in cm/s."))
	float RocketSpeed = 2500.0f;

	UPROPERTY(EditAnywhere, Category = "NavigationRocket", meta = (ClampMin = "200.0", ClampMax = "5000.0", UIMin = "200.0", UIMax = "5000.0", ToolTip = "How aggressively the rocket turns toward its target."))
	float HomingAcceleration = 1500.0f;

	UPROPERTY(EditAnywhere, Category = "NavigationRocket", meta = (ClampMin = "2.0", ClampMax = "15.0", UIMin = "2.0", UIMax = "15.0", ToolTip = "Seconds before rocket self-destructs if it does not hit."))
	float RocketLifetime = 8.0f;

	UPROPERTY(EditAnywhere, Category = "NavigationRocket", meta = (ClampMin = "50.0", ClampMax = "600.0", UIMin = "50.0", UIMax = "600.0", ToolTip = "Blast radius on impact."))
	float ExplosionRadius = 250.0f;

	UPROPERTY(EditAnywhere, Category = "NavigationRocket", meta = (ClampMin = "0.0", ClampMax = "10000.0", UIMin = "0.0", UIMax = "10000.0", ToolTip = "Sideways drift kick impulse applied at the rear."))
	float ImpactImpulse = 2000.0f;

	UPROPERTY(EditAnywhere, Category = "NavigationRocket", meta = (ClampMin = "0.0", ClampMax = "400.0", UIMin = "0.0", UIMax = "400.0", ToolTip = "Rearward offset where drift kick is applied."))
	float DriftKickRearOffset = 150.0f;

	UPROPERTY(EditAnywhere, Category = "NavigationRocket", meta = (ClampMin = "0.0", ClampMax = "10000.0", UIMin = "0.0", UIMax = "10000.0", ToolTip = "Downward impulse that keeps rocket hits grounded."))
	float DriftKickDownforceImpulse = 3500.0f;

	UPROPERTY(EditAnywhere, Category = "NavigationRocket", meta = (ClampMin = "0.0", ClampMax = "2000.0", UIMin = "0.0", UIMax = "2000.0", ToolTip = "Maximum upward velocity allowed after rocket hit."))
	float MaxUpwardVelocityAfterHit = 250.0f;

	UPROPERTY(EditAnywhere, Category = "NavigationRocket", meta = (ClampMin = "0.1", ClampMax = "5.0", UIMin = "0.5", UIMax = "5.0", ToolTip = "Seconds of reduced steering after a direct hit."))
	float SteeringReductionDuration = 1.5f;

	UPROPERTY(EditAnywhere, Category = "NavigationRocket|Visuals", meta = (ToolTip = "Visible rocket body."))
	UStaticMeshComponent* RocketMesh = nullptr;

	UPROPERTY(EditAnywhere, Category = "NavigationRocket|Visuals", meta = (ToolTip = "Particle trail on the rocket. Assign in editor."))
	UNiagaraComponent* TrailEffect = nullptr;

	UPROPERTY(EditAnywhere, Category = "NavigationRocket|Visuals", meta = (ToolTip = "Explosion particle effect. Assign in editor."))
	UNiagaraComponent* ExplosionEffect = nullptr;

	UPROPERTY(EditAnywhere, Category = "NavigationRocket|Visuals", meta = (ToolTip = "Sound played on explosion. Assign in editor."))
	USoundBase* ExplosionSound = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NavigationRocket")
	USphereComponent* CollisionSphere = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NavigationRocket")
	UProjectileMovementComponent* ProjectileMovement = nullptr;

	void Initialize(AHiveSportsCar* InSourceCar, AHiveSportsCar* InTargetCar, float InRocketSpeed, float InHomingAcceleration, float InRocketLifetime, float InExplosionRadius, float InImpactImpulse, float InDriftKickRearOffset, float InDriftKickDownforceImpulse, float InMaxUpwardVelocityAfterHit, float InSteeringReductionDuration, const FVector& FireDirection);

	virtual void BeginPlay() override;

protected:
	UFUNCTION()
	void OnRocketHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	void Explode(const FVector& HitLocation, AHiveSportsCar* DirectHitCar);
	void PlayExplosionEffects(const FVector& HitLocation);
	void FinishRocketLifetime();
	void HideRocketAfterImpact();
	void EnableHitRegistration();

	TWeakObjectPtr<AHiveSportsCar> SourceCar;
	FTimerHandle LifetimeTimerHandle;
	FTimerHandle HitGraceTimerHandle;
	bool bCanRegisterHit = false;
};

UCLASS(Blueprintable)
class HIVE_API ANavigationRocket : public APowerUpBase
{
	GENERATED_BODY()

public:
	ANavigationRocket();

	UPROPERTY(EditAnywhere, Category = "NavigationRocket", meta = (ClampMin = "500.0", ClampMax = "6000.0", UIMin = "500.0", UIMax = "6000.0", ToolTip = "Base travel speed of the rocket in cm/s."))
	float RocketSpeed = 2500.0f;

	UPROPERTY(EditAnywhere, Category = "NavigationRocket", meta = (ClampMin = "200.0", ClampMax = "5000.0", UIMin = "200.0", UIMax = "5000.0", ToolTip = "How aggressively the rocket turns toward its target."))
	float HomingAcceleration = 1500.0f;

	UPROPERTY(EditAnywhere, Category = "NavigationRocket", meta = (ClampMin = "2.0", ClampMax = "15.0", UIMin = "2.0", UIMax = "15.0", ToolTip = "Seconds before rocket self-destructs if it does not hit."))
	float RocketLifetime = 8.0f;

	UPROPERTY(EditAnywhere, Category = "NavigationRocket", meta = (ClampMin = "50.0", ClampMax = "600.0", UIMin = "50.0", UIMax = "600.0", ToolTip = "Blast radius on impact."))
	float ExplosionRadius = 250.0f;

	UPROPERTY(EditAnywhere, Category = "NavigationRocket", meta = (ClampMin = "0.0", ClampMax = "10000.0", UIMin = "0.0", UIMax = "10000.0", ToolTip = "Sideways drift kick impulse applied at the rear."))
	float ImpactImpulse = 2000.0f;

	UPROPERTY(EditAnywhere, Category = "NavigationRocket", meta = (ClampMin = "0.0", ClampMax = "400.0", UIMin = "0.0", UIMax = "400.0", ToolTip = "Rearward offset where drift kick is applied."))
	float DriftKickRearOffset = 150.0f;

	UPROPERTY(EditAnywhere, Category = "NavigationRocket", meta = (ClampMin = "0.0", ClampMax = "10000.0", UIMin = "0.0", UIMax = "10000.0", ToolTip = "Downward impulse that keeps rocket hits grounded."))
	float DriftKickDownforceImpulse = 3500.0f;

	UPROPERTY(EditAnywhere, Category = "NavigationRocket", meta = (ClampMin = "0.0", ClampMax = "2000.0", UIMin = "0.0", UIMax = "2000.0", ToolTip = "Maximum upward velocity allowed after rocket hit."))
	float MaxUpwardVelocityAfterHit = 250.0f;

	UPROPERTY(EditAnywhere, Category = "NavigationRocket", meta = (ClampMin = "0.1", ClampMax = "5.0", UIMin = "0.5", UIMax = "5.0", ToolTip = "Seconds of reduced steering after a direct hit."))
	float SteeringReductionDuration = 1.5f;

	UPROPERTY(EditAnywhere, Category = "NavigationRocket|Visuals", meta = (ToolTip = "Visible rocket body."))
	UStaticMeshComponent* RocketMesh = nullptr;

	UPROPERTY(EditAnywhere, Category = "NavigationRocket|Visuals", meta = (ToolTip = "Particle trail on the rocket. Assign in editor."))
	UNiagaraComponent* TrailEffect = nullptr;

	UPROPERTY(EditAnywhere, Category = "NavigationRocket|Visuals", meta = (ToolTip = "Explosion particle effect. Assign in editor."))
	UNiagaraComponent* ExplosionEffect = nullptr;

	UPROPERTY(EditAnywhere, Category = "NavigationRocket|Visuals", meta = (ToolTip = "Sound played on launch. Assign in editor."))
	USoundBase* LaunchSound = nullptr;

	UPROPERTY(EditAnywhere, Category = "NavigationRocket|Visuals", meta = (ToolTip = "Sound played on explosion. Assign in editor."))
	USoundBase* ExplosionSound = nullptr;

	virtual void OnActivate_Implementation(AHiveSportsCar* InstigatorCar) override;
};
