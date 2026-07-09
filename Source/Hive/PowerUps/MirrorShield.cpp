// Copyright Epic Games, Inc. All Rights Reserved.

#include "PowerUps/MirrorShield.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "HiveSportsCar.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

AMirrorShield::AMirrorShield()
{
	Category = EPowerUpCategory::SelfAffecting;
	DisplayName = FText::FromString(TEXT("Mirror Shield"));
	Duration = 2.5f;

	ShieldMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShieldMesh"));
	SetRootComponent(ShieldMesh);
	ShieldMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ShieldMesh->SetCanEverAffectNavigation(false);
	ShieldMesh->SetCastShadow(false);
	ShieldMesh->SetVisibility(false, true);
	ShieldMesh->SetRelativeScale3D(FVector(3.3f, 1.9f, 1.25f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMeshFinder(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMeshFinder.Succeeded())
	{
		ShieldMesh->SetStaticMesh(SphereMeshFinder.Object);
	}

	ShieldEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("ShieldEffect"));
	ShieldEffect->SetupAttachment(ShieldMesh);
	ShieldEffect->bAutoActivate = false;
}

void AMirrorShield::OnActivate_Implementation(AHiveSportsCar* InstigatorCar)
{
	AHiveSportsCar* TargetCar = InstigatorCar ? InstigatorCar : OwningCar;
	if (!TargetCar)
	{
		return;
	}

	Duration = ActiveDuration;
	bMirrorShieldInstanceActive = true;
	ShieldedCar = TargetCar;
	TargetCar->SetActiveMirrorShield(this);

	ShowShieldVisuals(TargetCar);

	if (ActivateSound)
	{
		UGameplayStatics::PlaySoundAtLocation(TargetCar, ActivateSound, TargetCar->GetActorLocation());
	}

	StartDurationTimer();
}

void AMirrorShield::OnExpire_Implementation()
{
	AHiveSportsCar* TargetCar = ShieldedCar.Get();
	if (TargetCar)
	{
		TargetCar->ClearActiveMirrorShield(this);
	}

	HideShieldVisuals();
	bMirrorShieldInstanceActive = false;
	ShieldedCar.Reset();

	Super::OnExpire_Implementation();
	SetLifeSpan(0.1f);
}

bool AMirrorShield::TryReflectProjectile(AActor* Projectile)
{
	if (!bMirrorShieldInstanceActive || !Projectile)
	{
		return false;
	}

	UProjectileMovementComponent* ProjectileMovement = Projectile->FindComponentByClass<UProjectileMovementComponent>();
	if (!ProjectileMovement)
	{
		return false;
	}

	FVector ReflectedVelocity = -ProjectileMovement->Velocity;
	if (ReflectedVelocity.IsNearlyZero())
	{
		ReflectedVelocity = -Projectile->GetActorForwardVector() * ProjectileMovement->InitialSpeed;
	}

	ProjectileMovement->Velocity = ReflectedVelocity;
	ProjectileMovement->UpdateComponentVelocity();
	Projectile->SetActorRotation(ReflectedVelocity.Rotation());

	if (AHiveSportsCar* TargetCar = ShieldedCar.Get())
	{
		if (UPrimitiveComponent* ProjectileCollision = Cast<UPrimitiveComponent>(Projectile->GetRootComponent()))
		{
			ProjectileCollision->IgnoreActorWhenMoving(TargetCar, true);
		}

		if (ReflectSound)
		{
			UGameplayStatics::PlaySoundAtLocation(TargetCar, ReflectSound, TargetCar->GetActorLocation());
		}
	}

	return true;
}

void AMirrorShield::ShowShieldVisuals(AHiveSportsCar* TargetCar)
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

void AMirrorShield::HideShieldVisuals()
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
