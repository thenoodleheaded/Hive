// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PowerUpTestStation.generated.h"

class AHiveSportsCar;
class APowerUpBase;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class HIVE_API APowerUpTestStation : public AActor
{
	GENERATED_BODY()

public:
	APowerUpTestStation();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TestStation")
	UStaticMeshComponent* StationMesh = nullptr;

	UPROPERTY(EditAnywhere, Category = "TestStation", meta = (ToolTip = "Power-up class this station fires at the test car."))
	TSubclassOf<APowerUpBase> PowerUpClassToFire;

	UPROPERTY(EditAnywhere, Category = "TestStation", meta = (ClampMin = "0.1", UIMin = "0.1", ToolTip = "Seconds between automatic power-up fires."))
	float FireInterval = 3.0f;

	UPROPERTY(EditAnywhere, Category = "TestStation", meta = (ToolTip = "If true, this station repeatedly fires while the level is playing."))
	bool bAutoFire = true;

	UPROPERTY(EditAnywhere, Category = "TestStation|NavigationRocket", meta = (ToolTip = "If true, this station overrides Navigation Rocket tuning values."))
	bool bOverrideNavigationRocketTuning = false;

	UPROPERTY(EditAnywhere, Category = "TestStation|NavigationRocket", meta = (EditCondition = "bOverrideNavigationRocketTuning", ClampMin = "500.0", ClampMax = "6000.0", UIMin = "500.0", UIMax = "6000.0", ToolTip = "Rocket travel speed. Higher needs more homing acceleration."))
	float NavigationRocketSpeed = 2500.0f;

	UPROPERTY(EditAnywhere, Category = "TestStation|NavigationRocket", meta = (EditCondition = "bOverrideNavigationRocketTuning", ClampMin = "200.0", ClampMax = "10000.0", UIMin = "200.0", UIMax = "10000.0", ToolTip = "Tracking precision. Higher turns harder toward the player."))
	float NavigationRocketHomingAcceleration = 3000.0f;

	UPROPERTY(EditAnywhere, Category = "TestStation|NavigationRocket", meta = (EditCondition = "bOverrideNavigationRocketTuning", ClampMin = "2.0", ClampMax = "15.0", UIMin = "2.0", UIMax = "15.0", ToolTip = "How long the rocket can chase before disappearing."))
	float NavigationRocketLifetime = 8.0f;

	UPROPERTY(EditAnywhere, Category = "TestStation|NavigationRocket", meta = (EditCondition = "bOverrideNavigationRocketTuning", ClampMin = "0.0", ClampMax = "10000.0", UIMin = "0.0", UIMax = "10000.0", ToolTip = "Impact force applied when the rocket explodes."))
	float NavigationRocketImpactImpulse = 2000.0f;

	UPROPERTY(EditAnywhere, Category = "TestStation|NavigationRocket", meta = (EditCondition = "bOverrideNavigationRocketTuning", ClampMin = "0.0", ClampMax = "400.0", UIMin = "0.0", UIMax = "400.0", ToolTip = "Rearward point where the rocket kick is applied."))
	float NavigationRocketDriftKickRearOffset = 150.0f;

	UPROPERTY(EditAnywhere, Category = "TestStation|NavigationRocket", meta = (EditCondition = "bOverrideNavigationRocketTuning", ClampMin = "0.0", ClampMax = "10000.0", UIMin = "0.0", UIMax = "10000.0", ToolTip = "Downward force that keeps the hit car grounded."))
	float NavigationRocketDriftKickDownforceImpulse = 3500.0f;

	UPROPERTY(EditAnywhere, Category = "TestStation|NavigationRocket", meta = (EditCondition = "bOverrideNavigationRocketTuning", ClampMin = "0.0", ClampMax = "2000.0", UIMin = "0.0", UIMax = "2000.0", ToolTip = "Maximum upward speed after the rocket hit."))
	float NavigationRocketMaxUpwardVelocityAfterHit = 250.0f;

	UFUNCTION(CallInEditor, BlueprintCallable, Category = "TestStation")
	void FireNow();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	AHiveSportsCar* FindTargetCar() const;

	FTimerHandle AutoFireTimerHandle;
};
