// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "UCarPhysicsProfile.generated.h"

UENUM(BlueprintType)
enum class ECarArchetype : uint8
{
	GripBiased,
	SlideBiased,
	Lightweight,
	Heavyweight,
	Balanced,
	EVArchetype
};

UENUM(BlueprintType)
enum class ECarClass : uint8
{
	Hypercar,
	Supercar,
	Sports,
	Muscle,
	HotHatch,
	Track,
	VanWildCard
};

UCLASS(BlueprintType)
class HIVE_API UCarPhysicsProfile : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Stats", meta = (ClampMin = "0.0", ClampMax = "100.0", UIMin = "0.0", UIMax = "100.0"))
	float Speed = 50.0f;

	UPROPERTY(EditAnywhere, Category = "Stats", meta = (ClampMin = "0.0", ClampMax = "100.0", UIMin = "0.0", UIMax = "100.0"))
	float Acceleration = 50.0f;

	UPROPERTY(EditAnywhere, Category = "Stats", meta = (ClampMin = "0.0", ClampMax = "100.0", UIMin = "0.0", UIMax = "100.0"))
	float Handling = 50.0f;

	UPROPERTY(EditAnywhere, Category = "Stats", meta = (ClampMin = "0.0", ClampMax = "100.0", UIMin = "0.0", UIMax = "100.0"))
	float Braking = 50.0f;

	UPROPERTY(EditAnywhere, Category = "Stats", meta = (ClampMin = "0.0", ClampMax = "100.0", UIMin = "0.0", UIMax = "100.0"))
	float Grip = 50.0f;

	UPROPERTY(EditAnywhere, Category = "Stats", meta = (ClampMin = "0.0", ClampMax = "100.0", UIMin = "0.0", UIMax = "100.0"))
	float Drift = 50.0f;

	UPROPERTY(EditAnywhere, Category = "Stats", meta = (ClampMin = "0.0", ClampMax = "100.0", UIMin = "0.0", UIMax = "100.0"))
	float Stability = 50.0f;

	UPROPERTY(EditAnywhere, Category = "Stats", meta = (ClampMin = "0.0", ClampMax = "100.0", UIMin = "0.0", UIMax = "100.0"))
	float Armor = 50.0f;

	UPROPERTY(EditAnywhere, Category = "Stats", meta = (ClampMin = "0.0", ClampMax = "100.0", UIMin = "0.0", UIMax = "100.0"))
	float Impact = 50.0f;

	UPROPERTY(EditAnywhere, Category = "Stats", meta = (ClampMin = "0.0", ClampMax = "100.0", UIMin = "0.0", UIMax = "100.0"))
	float Nitro = 50.0f;

	UPROPERTY(EditAnywhere, Category = "Archetype")
	ECarArchetype Archetype = ECarArchetype::Balanced;

	UPROPERTY(EditAnywhere, Category = "Classification")
	ECarClass Class = ECarClass::Sports;

	UPROPERTY(EditAnywhere, Category = "Hidden Values")
	float WeightKg = 1400.0f;

	UPROPERTY(EditAnywhere, Category = "Hidden Values")
	float WheelbaseMeters = 2.6f;

	UPROPERTY(EditAnywhere, Category = "Hidden Values")
	bool bIsEV = false;

	UPROPERTY(EditAnywhere, Category = "Hidden Values")
	int32 SlotCount = 2;

	UFUNCTION(BlueprintCallable, Category = "Stats")
	float GetNormalisedStat(FName StatName) const;
};
