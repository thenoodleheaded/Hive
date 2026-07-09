// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PowerUpBase.generated.h"

class AHiveSportsCar;
class UTexture2D;

UENUM(BlueprintType)
enum class EPowerUpCategory : uint8
{
	SelfAffecting,
	Projectile,
	AreaZone,
	TrackHazard,
	Positional
};

UCLASS(Abstract, Blueprintable)
class HIVE_API APowerUpBase : public AActor
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PowerUp")
	EPowerUpCategory Category = EPowerUpCategory::SelfAffecting;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PowerUp")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PowerUp", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float Duration = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PowerUp", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float Cooldown = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PowerUp")
	UTexture2D* HUDIcon = nullptr;

	UFUNCTION(BlueprintNativeEvent, Category = "PowerUp")
	void OnPickup(AHiveSportsCar* Carrier);

	UFUNCTION(BlueprintNativeEvent, Category = "PowerUp")
	void OnActivate(AHiveSportsCar* InstigatorCar);

	UFUNCTION(BlueprintNativeEvent, Category = "PowerUp")
	void OnExpire();

	void SetActivationSourceTransform(const FTransform& SourceTransform);
	void SetActivationContext(const FTransform& SourceTransform, AHiveSportsCar* SourceCar, AHiveSportsCar* TargetCar, AActor* SourceActor = nullptr);
	FVector GetActivationSourceLocation(AHiveSportsCar* FallbackCar) const;
	FVector GetActivationSourceForwardVector(AHiveSportsCar* FallbackCar) const;
	AHiveSportsCar* GetActivationSourceCar(AHiveSportsCar* FallbackCar) const;
	AActor* GetActivationSourceActor(AActor* FallbackActor) const;
	AHiveSportsCar* GetActivationTargetCar(AHiveSportsCar* FallbackCar) const;

protected:
	UPROPERTY()
	AHiveSportsCar* OwningCar = nullptr;

	FTimerHandle DurationTimerHandle;
	TOptional<FTransform> ActivationSourceTransform;
	bool bHasActivationSourceCarOverride = false;
	TWeakObjectPtr<AHiveSportsCar> ActivationSourceCarOverride;
	TWeakObjectPtr<AActor> ActivationSourceActorOverride;
	TWeakObjectPtr<AHiveSportsCar> ActivationTargetCarOverride;

	void StartDurationTimer();
	void HideAllVisualComponents();
};
