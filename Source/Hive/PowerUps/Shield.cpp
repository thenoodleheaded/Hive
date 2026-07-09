// Copyright Epic Games, Inc. All Rights Reserved.

#include "PowerUps/Shield.h"
#include "Components/StaticMeshComponent.h"
#include "Hive.h"
#include "HiveSportsCar.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

AShield::AShield()
{
	Category = EPowerUpCategory::SelfAffecting;
	DisplayName = FText::FromString(TEXT("Shield"));
	Duration = 6.0f;

	ShieldMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShieldMesh"));
	SetRootComponent(ShieldMesh);
	ShieldMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ShieldMesh->SetCanEverAffectNavigation(false);
	ShieldMesh->SetCastShadow(false);
	ShieldMesh->SetVisibility(false, true);
	ShieldMesh->SetRelativeScale3D(FVector(3.2f, 1.8f, 1.2f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMeshFinder(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMeshFinder.Succeeded())
	{
		ShieldMesh->SetStaticMesh(SphereMeshFinder.Object);
	}

	ShieldEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("ShieldEffect"));
	ShieldEffect->SetupAttachment(ShieldMesh);
	ShieldEffect->bAutoActivate = false;
}

void AShield::OnActivate_Implementation(AHiveSportsCar* InstigatorCar)
{
	AHiveSportsCar* TargetCar = InstigatorCar ? InstigatorCar : OwningCar;
	if (!TargetCar)
	{
		return;
	}

	Duration = ShieldDuration;
	HitsRemaining = MaxHitsAbsorbed;
	bShieldInstanceActive = true;
	ShieldedCar = TargetCar;
	TargetCar->SetActiveShield(this);

	ShowShieldVisuals(TargetCar);

	if (ActivateSound)
	{
		UGameplayStatics::PlaySoundAtLocation(TargetCar, ActivateSound, TargetCar->GetActorLocation());
	}

	StartDurationTimer();
}

void AShield::OnExpire_Implementation()
{
	if (bShieldInstanceActive)
	{
		UE_LOG(LogHive, Verbose, TEXT("[Shield] %s naturally expired after %.2fs with %d hit(s) remaining."),
			*GetNameSafe(ShieldedCar.Get()),
			ShieldDuration,
			HitsRemaining);

		BreakShield(false);
	}

	Super::OnExpire_Implementation();
	SetLifeSpan(0.1f);
}

bool AShield::TryAbsorbHit()
{
	if (!bShieldInstanceActive)
	{
		return false;
	}

	AHiveSportsCar* TargetCar = ShieldedCar.Get();
	if (TargetCar && HitAbsorbedSound)
	{
		UGameplayStatics::PlaySoundAtLocation(TargetCar, HitAbsorbedSound, TargetCar->GetActorLocation());
	}

	--HitsRemaining;
	const bool bBrokeThisHit = HitsRemaining <= 0;
	UE_LOG(LogHive, Verbose, TEXT("[Shield] Absorbed hit for %s. HitsRemaining=%d BrokeThisHit=%s."),
		*GetNameSafe(TargetCar),
		HitsRemaining,
		bBrokeThisHit ? TEXT("true") : TEXT("false"));

	if (HitsRemaining <= 0)
	{
		BreakShield(true);
		Super::OnExpire_Implementation();
		SetLifeSpan(0.1f);
	}

	return true;
}

void AShield::BreakShield(bool bPlayBrokenSound)
{
	if (!bShieldInstanceActive)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DurationTimerHandle);
	}

	AHiveSportsCar* TargetCar = ShieldedCar.Get();
	if (TargetCar)
	{
		TargetCar->ClearActiveShield(this);

		if (bPlayBrokenSound && ShieldBrokenSound)
		{
			UGameplayStatics::PlaySoundAtLocation(TargetCar, ShieldBrokenSound, TargetCar->GetActorLocation());
		}
	}

	HideShieldVisuals();
	bShieldInstanceActive = false;
	HitsRemaining = 0;
	ShieldedCar.Reset();
}

void AShield::ShowShieldVisuals(AHiveSportsCar* TargetCar)
{
	if (!TargetCar)
	{
		return;
	}

	AttachToActor(TargetCar, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	SetActorHiddenInGame(false);
	SetActorRelativeLocation(FVector::ZeroVector);

	if (ShieldMesh)
	{
		ShieldMesh->SetVisibility(true, true);
	}

	if (ShieldEffect)
	{
		ShieldEffect->SetVisibility(true, true);
		ShieldEffect->Activate(true);
	}
}

void AShield::HideShieldVisuals()
{
	if (ShieldMesh)
	{
		ShieldMesh->SetVisibility(false, true);
	}

	if (ShieldEffect)
	{
		ShieldEffect->Deactivate();
	}
}
