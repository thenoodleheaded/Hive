// Copyright Epic Games, Inc. All Rights Reserved.

#include "PowerUps/EMPBurst.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "Hive.h"
#include "HiveSportsCar.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraComponent.h"
#include "PowerUps/PowerUpSlotComponent.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"

AEMPBurst::AEMPBurst()
{
	Category = EPowerUpCategory::AreaZone;
	DisplayName = FText::FromString(TEXT("EMP Burst"));
	Duration = 0.0f;
}

void AEMPBurst::OnActivate_Implementation(AHiveSportsCar* InstigatorCar)
{
	AHiveSportsCar* TargetCar = GetActivationTargetCar(InstigatorCar ? InstigatorCar : OwningCar);
	if (!TargetCar || !GetWorld())
	{
		return;
	}

	AHiveSportsCar* SourceCar = GetActivationSourceCar(TargetCar);
	const FVector BlastLocation = GetActivationSourceLocation(TargetCar);
	PlayBlastVisuals(BlastLocation);

	TArray<FOverlapResult> Overlaps;
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Vehicle);
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(EMPBurstOverlap), false);

	GetWorld()->OverlapMultiByObjectType(
		Overlaps,
		BlastLocation,
		FQuat::Identity,
		ObjectQueryParams,
		FCollisionShape::MakeSphere(BlastRadius),
		QueryParams);

	TSet<AHiveSportsCar*> AffectedSportsCars;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AHiveSportsCar* SportsCar = Cast<AHiveSportsCar>(Overlap.GetActor());
		if (!SportsCar && Overlap.Component.IsValid())
		{
			SportsCar = Cast<AHiveSportsCar>(Overlap.Component->GetOwner());
		}

		if (!SportsCar)
		{
			continue;
		}

		if (SourceCar && !bAffectSelf && SportsCar == SourceCar)
		{
			continue;
		}

		AffectedSportsCars.Add(SportsCar);
	}

	for (AHiveSportsCar* SportsCar : AffectedSportsCars)
	{
		ApplyEMPToCar(SportsCar);
	}

	SetLifeSpan(FMath::Max(1.0f, DisableDuration + 0.5f));
}

void AEMPBurst::Destroyed()
{
	TArray<TWeakObjectPtr<AHiveSportsCar>> CarsToRestore;
	AffectedCars.GetKeys(CarsToRestore);

	for (const TWeakObjectPtr<AHiveSportsCar>& SportsCar : CarsToRestore)
	{
		RestoreCar(SportsCar.Get());
	}

	Super::Destroyed();
}

void AEMPBurst::ApplyEMPToCar(AHiveSportsCar* SportsCar)
{
	if (!SportsCar || !GetWorld())
	{
		return;
	}

	if (SportsCar->bIsEMPed)
	{
		UE_LOG(LogHive, Verbose, TEXT("[EMPBurst] Skipping %s because it is already EMPed."), *GetNameSafe(SportsCar));
		return;
	}

	TWeakObjectPtr<AHiveSportsCar> SportsCarKey(SportsCar);
	FEMPAffectedCarState& AffectedState = AffectedCars.FindOrAdd(SportsCarKey);

	SportsCar->bIsEMPed = true;

	if (UPowerUpSlotComponent* PowerUpSlots = SportsCar->GetPowerUpSlots())
	{
		PowerUpSlots->SetSlotsDisabled(true);
	}

	USkeletalMeshComponent* CarMesh = SportsCar->GetMesh();
	if (AffectedCarMaterial && CarMesh && AffectedState.OriginalMaterials.IsEmpty())
	{
		const int32 MaterialCount = CarMesh->GetNumMaterials();
		AffectedState.OriginalMaterials.Reserve(MaterialCount);

		for (int32 MaterialIndex = 0; MaterialIndex < MaterialCount; ++MaterialIndex)
		{
			AffectedState.OriginalMaterials.Add(CarMesh->GetMaterial(MaterialIndex));
			CarMesh->SetMaterial(MaterialIndex, AffectedCarMaterial);
		}
	}

	GetWorldTimerManager().ClearTimer(AffectedState.RestoreTimerHandle);
	FTimerDelegate RestoreDelegate = FTimerDelegate::CreateUObject(this, &AEMPBurst::RestoreCar, SportsCar);
	GetWorldTimerManager().SetTimer(AffectedState.RestoreTimerHandle, RestoreDelegate, DisableDuration, false);
}

void AEMPBurst::RestoreCar(AHiveSportsCar* SportsCar)
{
	TWeakObjectPtr<AHiveSportsCar> SportsCarKey(SportsCar);
	FEMPAffectedCarState* AffectedState = AffectedCars.Find(SportsCarKey);
	if (!AffectedState)
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(AffectedState->RestoreTimerHandle);

	if (SportsCar)
	{
		SportsCar->bIsEMPed = false;

		if (UPowerUpSlotComponent* PowerUpSlots = SportsCar->GetPowerUpSlots())
		{
			PowerUpSlots->SetSlotsDisabled(false);
		}

		if (USkeletalMeshComponent* CarMesh = SportsCar->GetMesh())
		{
			for (int32 MaterialIndex = 0; MaterialIndex < AffectedState->OriginalMaterials.Num(); ++MaterialIndex)
			{
				CarMesh->SetMaterial(MaterialIndex, AffectedState->OriginalMaterials[MaterialIndex]);
			}
		}
	}

	AffectedCars.Remove(SportsCarKey);
}

void AEMPBurst::PlayBlastVisuals(const FVector& BlastLocation)
{
	if (BlastEffect)
	{
		BlastEffect->SetWorldLocation(BlastLocation);
		BlastEffect->SetVisibility(true, true);
		BlastEffect->Activate(true);
	}

	if (BlastSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, BlastSound, BlastLocation);
	}
}
