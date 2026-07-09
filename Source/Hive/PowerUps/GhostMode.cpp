// Copyright Epic Games, Inc. All Rights Reserved.

#include "PowerUps/GhostMode.h"
#include "Components/SkeletalMeshComponent.h"
#include "HiveSportsCar.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraComponent.h"
#include "Sound/SoundBase.h"

AGhostMode::AGhostMode()
{
	Category = EPowerUpCategory::SelfAffecting;
	DisplayName = FText::FromString(TEXT("Ghost Mode"));
	Duration = 4.0f;
}

void AGhostMode::OnActivate_Implementation(AHiveSportsCar* InstigatorCar)
{
	AHiveSportsCar* TargetCar = InstigatorCar ? InstigatorCar : OwningCar;
	if (!TargetCar)
	{
		return;
	}

	USkeletalMeshComponent* CarMesh = TargetCar->GetMesh();
	if (!CarMesh)
	{
		return;
	}

	GhostedCar = TargetCar;
	Duration = GhostDuration;

	StoreOriginalMeshState(CarMesh);

	CarMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CarMesh->SetCollisionResponseToChannel(ECC_Vehicle, ECR_Overlap);
	ApplyGhostVisuals(CarMesh);

	if (ActivateSound)
	{
		UGameplayStatics::PlaySoundAtLocation(TargetCar, ActivateSound, TargetCar->GetActorLocation());
	}

	if (GhostEffect)
	{
		GhostEffect->SetWorldLocation(TargetCar->GetActorLocation());
		GhostEffect->SetVisibility(true, true);
		GhostEffect->Activate(true);
	}

	StartDurationTimer();
}

void AGhostMode::OnExpire_Implementation()
{
	AHiveSportsCar* TargetCar = GhostedCar.Get();
	if (TargetCar)
	{
		if (USkeletalMeshComponent* CarMesh = TargetCar->GetMesh())
		{
			RestoreOriginalMeshState(CarMesh);
		}

		if (ExpireSound)
		{
			UGameplayStatics::PlaySoundAtLocation(TargetCar, ExpireSound, TargetCar->GetActorLocation());
		}
	}

	if (GhostEffect)
	{
		GhostEffect->Deactivate();
		GhostEffect->SetVisibility(false, true);
	}

	GhostedCar.Reset();
	Super::OnExpire_Implementation();
	SetLifeSpan(0.1f);
}

void AGhostMode::StoreOriginalMeshState(USkeletalMeshComponent* CarMesh)
{
	if (!CarMesh)
	{
		return;
	}

	OriginalPawnResponse = CarMesh->GetCollisionResponseToChannel(ECC_Pawn);
	OriginalVehicleResponse = CarMesh->GetCollisionResponseToChannel(ECC_Vehicle);
	bOriginalHiddenInGame = CarMesh->bHiddenInGame;

	OriginalMaterials.Reset();
	const int32 MaterialCount = CarMesh->GetNumMaterials();
	OriginalMaterials.Reserve(MaterialCount);

	for (int32 MaterialIndex = 0; MaterialIndex < MaterialCount; ++MaterialIndex)
	{
		OriginalMaterials.Add(CarMesh->GetMaterial(MaterialIndex));
	}
}

void AGhostMode::ApplyGhostVisuals(USkeletalMeshComponent* CarMesh)
{
	if (!CarMesh)
	{
		return;
	}

	if (GhostMaterial)
	{
		const int32 MaterialCount = CarMesh->GetNumMaterials();
		for (int32 MaterialIndex = 0; MaterialIndex < MaterialCount; ++MaterialIndex)
		{
			CarMesh->SetMaterial(MaterialIndex, GhostMaterial);
		}

		return;
	}

	CarMesh->SetHiddenInGame(true, true);
}

void AGhostMode::RestoreOriginalMeshState(USkeletalMeshComponent* CarMesh)
{
	if (!CarMesh)
	{
		return;
	}

	CarMesh->SetCollisionResponseToChannel(ECC_Pawn, OriginalPawnResponse);
	CarMesh->SetCollisionResponseToChannel(ECC_Vehicle, OriginalVehicleResponse);
	CarMesh->SetHiddenInGame(bOriginalHiddenInGame, true);

	for (int32 MaterialIndex = 0; MaterialIndex < OriginalMaterials.Num(); ++MaterialIndex)
	{
		CarMesh->SetMaterial(MaterialIndex, OriginalMaterials[MaterialIndex]);
	}

	OriginalMaterials.Reset();
}
