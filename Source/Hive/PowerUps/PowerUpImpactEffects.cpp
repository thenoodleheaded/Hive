// Copyright Epic Games, Inc. All Rights Reserved.

#include "PowerUps/PowerUpImpactEffects.h"
#include "Components/SkeletalMeshComponent.h"
#include "EngineUtils.h"
#include "Hive.h"
#include "HiveSportsCar.h"
#include "HiveVehicleMovementComponent.h"

namespace HivePowerUpImpactEffects
{
void ApplyProjectileImpact(UObject* WorldContextObject, const FVector& HitLocation, const FVector& ProjectileVelocity, AHiveSportsCar* DirectHitCar, const FHiveProjectileImpactParams& Params, const TCHAR* LogPrefix)
{
	UE_LOG(LogHive, Verbose, TEXT("[%s] SharedImpact begin HitLocation=%s ProjectileVelocity=%s DirectHitCar=%s Radius=%.1f Impact=%.1f RearOffset=%.1f Downforce=%.1f MaxUpZ=%.1f SteeringDuration=%.2f"),
		LogPrefix,
		*HitLocation.ToCompactString(),
		*ProjectileVelocity.ToCompactString(),
		*GetNameSafe(DirectHitCar),
		Params.ExplosionRadius,
		Params.ImpactImpulse,
		Params.DriftKickRearOffset,
		Params.DriftKickDownforceImpulse,
		Params.MaxUpwardVelocityAfterHit,
		Params.SteeringReductionDuration);

	if (DirectHitCar)
	{
		if (UHiveVehicleMovementComponent* HiveMovement = Cast<UHiveVehicleMovementComponent>(DirectHitCar->GetChaosVehicleMovement()))
		{
			HiveMovement->ForceDriftFromExternalImpact();
			HiveMovement->ApplyTemporarySteeringMultiplier(Params.SteeringReductionMultiplier, Params.SteeringReductionDuration);
			UE_LOG(LogHive, Verbose, TEXT("[%s] Applied steering reduction %.2f for %.2fs to %s."),
				LogPrefix,
				Params.SteeringReductionMultiplier,
				Params.SteeringReductionDuration,
				*GetNameSafe(DirectHitCar));
		}
	}

	UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	if (!World)
	{
		return;
	}

	for (TActorIterator<AHiveSportsCar> It(World); It; ++It)
	{
		AHiveSportsCar* SportsCar = *It;
		if (!SportsCar)
		{
			continue;
		}

		const float Distance = FVector::Dist(SportsCar->GetActorLocation(), HitLocation);
		if (Distance > Params.ExplosionRadius)
		{
			continue;
		}

		USkeletalMeshComponent* CarMesh = SportsCar->GetMesh();
		if (!CarMesh)
		{
			continue;
		}

		const float Falloff = Params.ExplosionRadius > KINDA_SMALL_NUMBER ? FMath::Clamp(1.0f - (Distance / Params.ExplosionRadius), 0.0f, 1.0f) : 1.0f;
		const FVector CarLocation = SportsCar->GetActorLocation();
		const FVector CarForward = SportsCar->GetActorForwardVector().GetSafeNormal2D();
		const FVector CarRight = SportsCar->GetActorRightVector().GetSafeNormal2D();
		const FVector CarUp = SportsCar->GetActorUpVector().GetSafeNormal();

		FVector ImpactToCar = CarLocation - HitLocation;
		ImpactToCar.Z = 0.0f;
		ImpactToCar.Normalize();

		float SideDot = FVector::DotProduct(ImpactToCar, CarRight);
		if (FMath::Abs(SideDot) < 0.1f)
		{
			FVector ProjectileDirection = ProjectileVelocity;
			ProjectileDirection.Z = 0.0f;
			ProjectileDirection.Normalize();
			SideDot = FVector::DotProduct(ProjectileDirection, CarRight);
		}

		const float SideSign = SideDot >= 0.0f ? 1.0f : -1.0f;
		const float AngleStrength = FMath::Clamp(FMath::Abs(SideDot), 0.35f, 1.0f);
		const FVector RearKickLocation = CarLocation - CarForward * Params.DriftKickRearOffset;
		const float BodyMass = FMath::Max(1.0f, CarMesh->GetBoneMass());
		const FVector SideImpulse = CarRight * SideSign * Params.ImpactImpulse * AngleStrength * Falloff;
		const FVector MassScaledSideImpulse = SideImpulse * BodyMass;
		const FVector DownforceImpulse = -CarUp * Params.DriftKickDownforceImpulse * Falloff;

		CarMesh->AddImpulseAtLocation(MassScaledSideImpulse, RearKickLocation, NAME_None);
		CarMesh->AddImpulse(DownforceImpulse, NAME_None, true);

		if (Params.MaxUpwardVelocityAfterHit >= 0.0f)
		{
			FVector CurrentVelocity = CarMesh->GetPhysicsLinearVelocity();
			CurrentVelocity.Z = FMath::Min(CurrentVelocity.Z, Params.MaxUpwardVelocityAfterHit);
			CarMesh->SetPhysicsLinearVelocity(CurrentVelocity);
		}

		UE_LOG(LogHive, Verbose, TEXT("[%s] Applying drift kick to %s Distance=%.1f Falloff=%.2f SideDot=%.2f BodyMass=%.1f SideVelocityKick=%s RearPoint=%s Downforce=%s MaxUpZ=%.1f"),
			LogPrefix,
			*GetNameSafe(SportsCar),
			Distance,
			Falloff,
			SideDot,
			BodyMass,
			*SideImpulse.ToCompactString(),
			*RearKickLocation.ToCompactString(),
			*DownforceImpulse.ToCompactString(),
			Params.MaxUpwardVelocityAfterHit);
	}
}
}
