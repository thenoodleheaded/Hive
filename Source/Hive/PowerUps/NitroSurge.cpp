// Copyright Epic Games, Inc. All Rights Reserved.

#include "PowerUps/NitroSurge.h"
#include "Components/SkeletalMeshComponent.h"
#include "Hive.h"
#include "HiveSportsCar.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraComponent.h"
#include "PhysicsEngine/BodyInstance.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"

ANitroSurge::ANitroSurge()
{
	Category = EPowerUpCategory::SelfAffecting;
	DisplayName = FText::FromString(TEXT("Nitro Surge"));
	Duration = 0.0f;
}

void ANitroSurge::OnPickup_Implementation(AHiveSportsCar* Carrier)
{
	Super::OnPickup_Implementation(Carrier);

	DisplayName = FText::FromString(TEXT("Nitro Surge"));
	UE_LOG(LogHive, Verbose, TEXT("Nitro Surge picked up by %s."), *GetNameSafe(Carrier));
}

void ANitroSurge::OnActivate_Implementation(AHiveSportsCar* InstigatorCar)
{
	AHiveSportsCar* TargetCar = InstigatorCar ? InstigatorCar : OwningCar;
	if (!TargetCar)
	{
		return;
	}

	USkeletalMeshComponent* CarMesh = TargetCar->GetMesh();
	FBodyInstance* BodyInstance = CarMesh ? CarMesh->GetBodyInstance() : nullptr;
	if (BodyInstance)
	{
		const FVector Impulse = TargetCar->GetActorForwardVector() * ImpulseStrength * BodyInstance->GetBodyMass();
		BodyInstance->AddImpulse(Impulse, false);
	}

	if (NitroEffect)
	{
		NitroEffect->SetWorldLocation(TargetCar->GetActorLocation());
		NitroEffect->SetVisibility(true, true);
		NitroEffect->Activate(true);
	}

	if (NitroSound)
	{
		UGameplayStatics::PlaySoundAtLocation(TargetCar, NitroSound, TargetCar->GetActorLocation());
	}

	if (NitroFlashMaterial && CarMesh)
	{
		OriginalMaterials.Reset();

		const int32 MaterialCount = CarMesh->GetNumMaterials();
		for (int32 MaterialIndex = 0; MaterialIndex < MaterialCount; ++MaterialIndex)
		{
			OriginalMaterials.Add(CarMesh->GetMaterial(MaterialIndex));
			CarMesh->SetMaterial(MaterialIndex, NitroFlashMaterial);
		}

		GetWorldTimerManager().SetTimer(NitroFlashTimerHandle, this, &ANitroSurge::RestoreNitroFlashMaterial, 0.2f, false);
	}

	StartDurationTimer();
	SetLifeSpan(1.0f);
}

void ANitroSurge::OnExpire_Implementation()
{
	if (NitroEffect)
	{
		NitroEffect->Deactivate();
		NitroEffect->SetVisibility(false, true);
	}

	Super::OnExpire_Implementation();
}

void ANitroSurge::RestoreNitroFlashMaterial()
{
	if (!OwningCar)
	{
		return;
	}

	USkeletalMeshComponent* CarMesh = OwningCar->GetMesh();
	if (!CarMesh)
	{
		return;
	}

	for (int32 MaterialIndex = 0; MaterialIndex < OriginalMaterials.Num(); ++MaterialIndex)
	{
		CarMesh->SetMaterial(MaterialIndex, OriginalMaterials[MaterialIndex]);
	}

	OriginalMaterials.Reset();
}
