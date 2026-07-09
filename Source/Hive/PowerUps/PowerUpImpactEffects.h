// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class AHiveSportsCar;

struct FHiveProjectileImpactParams
{
	float ExplosionRadius = 250.0f;
	float ImpactImpulse = 2000.0f;
	float DriftKickRearOffset = 150.0f;
	float DriftKickDownforceImpulse = 3500.0f;
	float MaxUpwardVelocityAfterHit = 250.0f;
	float SteeringReductionDuration = 1.5f;
	float SteeringReductionMultiplier = 0.3f;
};

namespace HivePowerUpImpactEffects
{
	void ApplyProjectileImpact(UObject* WorldContextObject, const FVector& HitLocation, const FVector& ProjectileVelocity, AHiveSportsCar* DirectHitCar, const FHiveProjectileImpactParams& Params, const TCHAR* LogPrefix);
}
