// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PowerUps/PowerUpBase.h"
#include "OilSlick.generated.h"

class AHiveSportsCar;
class UHiveVehicleMovementComponent;
class UNiagaraComponent;
class USoundBase;
class USphereComponent;
class UStaticMeshComponent;

struct FSlickAffectedCarState
{
	int32 AffectedWheelCount = 0;
	FTimerHandle RestoreTimerHandle;
};

UCLASS(Blueprintable)
class HIVE_API ASlickZoneActor : public AActor
{
	GENERATED_BODY()

public:
	ASlickZoneActor();

	UPROPERTY(EditAnywhere, Category = "OilSlick", meta = (ClampMin = "100.0", ClampMax = "2000.0", UIMin = "100.0", UIMax = "2000.0", ToolTip = "Radius of the oil slick zone in cm."))
	float SlickRadius = 1200.0f;

	UPROPERTY(EditAnywhere, Category = "OilSlick", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", ToolTip = "Friction multiplier applied to cars in the slick. Lower = more slippery."))
	float GripReduction = 0.15f;

	UPROPERTY(EditAnywhere, Category = "OilSlick", meta = (ClampMin = "2.0", ClampMax = "60.0", UIMin = "2.0", UIMax = "60.0", ToolTip = "How long the oil slick stays on the track before disappearing."))
	float SlickDuration = 12.0f;

	UPROPERTY(EditAnywhere, Category = "OilSlick", meta = (ClampMin = "0.5", ClampMax = "10.0", UIMin = "0.5", UIMax = "10.0", ToolTip = "How long grip reduction lasts on a car after driving through the slick."))
	float EffectDuration = 3.0f;

	UPROPERTY(EditAnywhere, Category = "OilSlick|Visuals", meta = (ToolTip = "Flat decal-like mesh showing the oil puddle."))
	UStaticMeshComponent* SlickMesh = nullptr;

	UPROPERTY(EditAnywhere, Category = "OilSlick|Visuals", meta = (ToolTip = "Particle effect for the slick. Assign in editor."))
	UNiagaraComponent* SlickEffect = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "OilSlick")
	USphereComponent* CollisionSphere = nullptr;

	void Initialize(float InSlickRadius, float InGripReduction, float InSlickDuration, float InEffectDuration);
	void ApplyToCar(AHiveSportsCar* SportsCar);

	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Destroyed() override;

protected:
	UFUNCTION()
	void OnSlickOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	void ApplyGripReduction(TWeakObjectPtr<AHiveSportsCar> SportsCar);
	void RestoreCarGrip(TWeakObjectPtr<AHiveSportsCar> SportsCar);
	void DestroySlickZone();
	void UpdateZoneVisualScale();

	TMap<TWeakObjectPtr<AHiveSportsCar>, FSlickAffectedCarState> AffectedCars;
	FTimerHandle SlickLifetimeTimerHandle;
};

UCLASS(Blueprintable)
class HIVE_API AOilSlick : public APowerUpBase
{
	GENERATED_BODY()

public:
	AOilSlick();

	UPROPERTY(EditAnywhere, Category = "OilSlick", meta = (ClampMin = "100.0", ClampMax = "2000.0", UIMin = "100.0", UIMax = "2000.0", ToolTip = "Radius of the oil slick zone in cm."))
	float SlickRadius = 1200.0f;

	UPROPERTY(EditAnywhere, Category = "OilSlick", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", ToolTip = "Friction multiplier applied to cars in the slick. Lower = more slippery."))
	float GripReduction = 0.15f;

	UPROPERTY(EditAnywhere, Category = "OilSlick", meta = (ClampMin = "2.0", ClampMax = "60.0", UIMin = "2.0", UIMax = "60.0", ToolTip = "How long the oil slick stays on the track before disappearing."))
	float SlickDuration = 12.0f;

	UPROPERTY(EditAnywhere, Category = "OilSlick", meta = (ClampMin = "0.5", ClampMax = "10.0", UIMin = "0.5", UIMax = "10.0", ToolTip = "How long grip reduction lasts on a car after driving through the slick."))
	float EffectDuration = 3.0f;

	UPROPERTY(EditAnywhere, Category = "OilSlick|Visuals", meta = (ToolTip = "Sound played when slick is deployed. Assign in editor."))
	USoundBase* DeploySound = nullptr;

	virtual void OnActivate_Implementation(AHiveSportsCar* InstigatorCar) override;
};
