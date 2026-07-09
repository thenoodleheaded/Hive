// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PowerUps/PowerUpBase.h"
#include "SlowField.generated.h"

class AHiveSportsCar;
class UAudioComponent;
class UNiagaraComponent;
class USoundBase;
class UBoxComponent;
class UStaticMeshComponent;

struct FSlowFieldAffectedCarState
{
	float EntrySpeed = 0.0f;
	float SpeedCap = 0.0f;
};

UCLASS(Blueprintable)
class HIVE_API ASlowFieldActor : public AActor
{
	GENERATED_BODY()

public:
	ASlowFieldActor();

	UPROPERTY(EditAnywhere, Category = "SlowField", meta = (ClampMin = "0.0", ToolTip = "Half-size of the slow field box in cm."))
	FVector FieldExtent = FVector(600.0f, 600.0f, 150.0f);

	UPROPERTY(EditAnywhere, Category = "SlowField", meta = (ClampMin = "1.0", ClampMax = "15.0", UIMin = "1.0", UIMax = "15.0", ToolTip = "How long the slow field stays active."))
	float FieldDuration = 10.0f;

	UPROPERTY(EditAnywhere, Category = "SlowField", meta = (ClampMin = "0.1", ClampMax = "0.9", UIMin = "0.1", UIMax = "0.9", ToolTip = "Speed fraction applied to cars in the field. 0.7 = 70 percent speed."))
	float SpeedMultiplier = 0.7f;

	UPROPERTY(EditAnywhere, Category = "SlowField", meta = (ToolTip = "If true, the field also affects the car that deployed it."))
	bool bAffectSelf = false;

	UPROPERTY(EditAnywhere, Category = "SlowField|Visuals", meta = (ClampMin = "1.0", UIMin = "1.0", ToolTip = "Thickness of the visible field boundary edges in cm."))
	float EdgeThickness = 12.0f;

	UPROPERTY(EditAnywhere, Category = "SlowField|Visuals", meta = (ToolTip = "Atmospheric particle effect inside the zone. Assign in editor."))
	UNiagaraComponent* FieldEffect = nullptr;

	UPROPERTY(EditAnywhere, Category = "SlowField|Visuals", meta = (ToolTip = "Audio loop while field is active. Assign in editor."))
	USoundBase* LoopSound = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SlowField")
	UBoxComponent* CollisionBox = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SlowField|Visuals")
	TArray<TObjectPtr<UStaticMeshComponent>> EdgeMeshes;

	void Initialize(AHiveSportsCar* InSourceCar, const FVector& InFieldExtent, float InFieldDuration, float InSpeedMultiplier, bool bInAffectSelf, USoundBase* InLoopSound);
	void ApplySlowToCar(AHiveSportsCar* SportsCar);

	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Destroyed() override;

protected:
	UFUNCTION()
	void OnSlowFieldBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnSlowFieldEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void RefreshAffectedCars();
	void RestoreCarSpeed(AHiveSportsCar* SportsCar);
	void DestroySlowField();
	void UpdateFieldVisualScale();

	TWeakObjectPtr<AHiveSportsCar> SourceCar;
	TMap<TWeakObjectPtr<AHiveSportsCar>, FSlowFieldAffectedCarState> AffectedCars;
	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> LoopAudioComponent = nullptr;
	FTimerHandle FieldLifetimeTimerHandle;
	bool bHasInitialized = false;
};

UCLASS(Blueprintable)
class HIVE_API ASlowField : public APowerUpBase
{
	GENERATED_BODY()

public:
	ASlowField();

	UPROPERTY(EditAnywhere, Category = "SlowField", meta = (ClampMin = "0.0", ToolTip = "Half-size of the slow field box in cm."))
	FVector FieldExtent = FVector(600.0f, 600.0f, 150.0f);

	UPROPERTY(EditAnywhere, Category = "SlowField", meta = (ClampMin = "1.0", ClampMax = "15.0", UIMin = "1.0", UIMax = "15.0", ToolTip = "How long the slow field stays active."))
	float FieldDuration = 10.0f;

	UPROPERTY(EditAnywhere, Category = "SlowField", meta = (ClampMin = "0.1", ClampMax = "0.9", UIMin = "0.1", UIMax = "0.9", ToolTip = "Speed fraction applied to cars in the field. 0.7 = 70 percent speed."))
	float SpeedMultiplier = 0.7f;

	UPROPERTY(EditAnywhere, Category = "SlowField", meta = (ToolTip = "If true, the field also affects the car that deployed it."))
	bool bAffectSelf = false;

	UPROPERTY(EditAnywhere, Category = "SlowField|Visuals", meta = (ToolTip = "Atmospheric particle effect inside the zone. Assign in editor."))
	UNiagaraComponent* FieldEffect = nullptr;

	UPROPERTY(EditAnywhere, Category = "SlowField|Visuals", meta = (ToolTip = "Sound played when field is deployed. Assign in editor."))
	USoundBase* DeploySound = nullptr;

	UPROPERTY(EditAnywhere, Category = "SlowField|Visuals", meta = (ToolTip = "Audio loop while field is active. Assign in editor."))
	USoundBase* LoopSound = nullptr;

	virtual void OnActivate_Implementation(AHiveSportsCar* InstigatorCar) override;
};
