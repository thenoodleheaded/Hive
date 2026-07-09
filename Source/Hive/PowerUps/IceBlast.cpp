// Copyright Epic Games, Inc. All Rights Reserved.

#include "PowerUps/IceBlast.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Hive.h"
#include "HiveSportsCar.h"
#include "HiveVehicleMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

AIceBlastProjectile::AIceBlastProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	SetRootComponent(CollisionSphere);
	CollisionSphere->InitSphereRadius(ProjectileRadius);
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CollisionSphere->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionSphere->SetCollisionResponseToAllChannels(ECR_Block);
	CollisionSphere->SetGenerateOverlapEvents(false);
	CollisionSphere->SetNotifyRigidBodyCollision(false);
	CollisionSphere->SetCanEverAffectNavigation(false);

	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileMesh"));
	ProjectileMesh->SetupAttachment(CollisionSphere);
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ProjectileMesh->SetCanEverAffectNavigation(false);
	ProjectileMesh->SetCastShadow(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMeshFinder(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMeshFinder.Succeeded())
	{
		ProjectileMesh->SetStaticMesh(SphereMeshFinder.Object);
		ProjectileMesh->SetRelativeScale3D(FVector(0.3f));
	}

	TrailEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("TrailEffect"));
	TrailEffect->SetupAttachment(CollisionSphere);
	TrailEffect->bAutoActivate = false;

	HitEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("HitEffect"));
	HitEffect->SetupAttachment(CollisionSphere);
	HitEffect->bAutoActivate = false;

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->InitialSpeed = ProjectileSpeed;
	ProjectileMovement->MaxSpeed = ProjectileSpeed;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = false;
	ProjectileMovement->ProjectileGravityScale = 0.0f;
}

void AIceBlastProjectile::Initialize(AHiveSportsCar* InSourceCar, float InProjectileSpeed, float InFreezeDuration, float InProjectileLifetime, float InProjectileRadius, const FVector& FireDirection)
{
	SourceCar = InSourceCar;
	ProjectileSpeed = InProjectileSpeed;
	FreezeDuration = InFreezeDuration;
	ProjectileLifetime = InProjectileLifetime;
	ProjectileRadius = InProjectileRadius;

	if (CollisionSphere)
	{
		CollisionSphere->SetSphereRadius(ProjectileRadius);
		if (InSourceCar)
		{
			CollisionSphere->IgnoreActorWhenMoving(InSourceCar, true);
		}
	}

	if (ProjectileMovement)
	{
		const FVector LaunchDirection = FireDirection.GetSafeNormal();
		ProjectileMovement->InitialSpeed = ProjectileSpeed;
		ProjectileMovement->MaxSpeed = ProjectileSpeed;
		ProjectileMovement->Velocity = LaunchDirection * ProjectileSpeed;
		ProjectileMovement->Activate(true);
	}
}

void AIceBlastProjectile::BeginPlay()
{
	Super::BeginPlay();

	if (CollisionSphere)
	{
		CollisionSphere->OnComponentHit.AddDynamic(this, &AIceBlastProjectile::OnProjectileHit);
		CollisionSphere->SetSphereRadius(ProjectileRadius);
		CollisionSphere->SetNotifyRigidBodyCollision(false);
	}

	if (TrailEffect)
	{
		TrailEffect->Activate(true);
	}

	GetWorldTimerManager().SetTimer(HitNotificationTimerHandle, this, &AIceBlastProjectile::EnableProjectileHitNotifications, 0.1f, false);
	GetWorldTimerManager().SetTimer(LifetimeTimerHandle, this, &AIceBlastProjectile::FinishProjectileLifetime, ProjectileLifetime, false);
}

void AIceBlastProjectile::OnProjectileHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (!OtherActor || OtherActor == this)
	{
		return;
	}

	AHiveSportsCar* HitCar = Cast<AHiveSportsCar>(OtherActor);
	if (!HitCar || HitCar == SourceCar.Get())
	{
		PlayHitEffects(Hit.ImpactPoint);
		Destroy();
		return;
	}

	if (HitCar->TryReflectProjectile(this))
	{
		if (CollisionSphere && SourceCar.IsValid())
		{
			CollisionSphere->IgnoreActorWhenMoving(SourceCar.Get(), false);
		}

		SourceCar = HitCar;
		return;
	}

	PlayHitEffects(Hit.ImpactPoint);

	if (HitCar->TryAbsorbHit())
	{
		Destroy();
		return;
	}

	FrozenCar = HitCar;
	HideProjectileAfterHit();
	ApplyFullInputFreeze();

	GetWorldTimerManager().ClearTimer(FreezeEndTimerHandle);
	GetWorldTimerManager().SetTimer(FreezeEndTimerHandle, this, &AIceBlastProjectile::EndInputFreeze, FreezeDuration, false);
}

void AIceBlastProjectile::ApplyFullInputFreeze()
{
	AHiveSportsCar* HitCar = FrozenCar.Get();
	if (!HitCar)
	{
		EndInputFreeze();
		return;
	}

	if (UHiveVehicleMovementComponent* Movement = Cast<UHiveVehicleMovementComponent>(HitCar->GetChaosVehicleMovement()))
	{
		Movement->ApplyFullInputFreeze(FreezeDuration);
	}
}

void AIceBlastProjectile::EndInputFreeze()
{
	GetWorldTimerManager().ClearTimer(FreezeEndTimerHandle);
	FrozenCar.Reset();
	Destroy();
}

void AIceBlastProjectile::FinishProjectileLifetime()
{
	Destroy();
}

void AIceBlastProjectile::EnableProjectileHitNotifications()
{
	if (CollisionSphere)
	{
		CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		CollisionSphere->SetNotifyRigidBodyCollision(true);
	}
}

void AIceBlastProjectile::PlayHitEffects(const FVector& HitLocation)
{
	if (TrailEffect)
	{
		TrailEffect->Deactivate();
	}

	if (HitEffect && HitEffect->GetAsset())
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, HitEffect->GetAsset(), HitLocation);
	}

	if (HitSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, HitSound, HitLocation);
	}
}

void AIceBlastProjectile::HideProjectileAfterHit()
{
	GetWorldTimerManager().ClearTimer(LifetimeTimerHandle);

	if (CollisionSphere)
	{
		CollisionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (ProjectileMesh)
	{
		ProjectileMesh->SetVisibility(false, true);
	}

	if (ProjectileMovement)
	{
		ProjectileMovement->StopMovementImmediately();
		ProjectileMovement->Deactivate();
	}
}

AIceBlast::AIceBlast()
{
	Category = EPowerUpCategory::Projectile;
	DisplayName = FText::FromString(TEXT("Ice Blast"));
	Duration = 0.0f;
}

void AIceBlast::OnActivate_Implementation(AHiveSportsCar* InstigatorCar)
{
	AHiveSportsCar* TargetCar = GetActivationTargetCar(InstigatorCar ? InstigatorCar : OwningCar);
	if (!TargetCar || !GetWorld())
	{
		return;
	}

	AHiveSportsCar* SourceCar = GetActivationSourceCar(TargetCar);
	const FVector FireDirection = GetActivationSourceForwardVector(TargetCar);
	const FVector SpawnLocation = GetActivationSourceLocation(TargetCar) + FireDirection * 300.0f + FVector(0.0f, 0.0f, SpawnHeightOffset);
	const FRotator SpawnRotation = FireDirection.Rotation();

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = SourceCar ? SourceCar : TargetCar;
	SpawnParameters.Instigator = SourceCar ? SourceCar : TargetCar;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AIceBlastProjectile* Projectile = GetWorld()->SpawnActor<AIceBlastProjectile>(AIceBlastProjectile::StaticClass(), SpawnLocation, SpawnRotation, SpawnParameters);
	if (Projectile)
	{
		Projectile->Initialize(SourceCar, ProjectileSpeed, FreezeDuration, ProjectileLifetime, ProjectileRadius, FireDirection);

		if (ProjectileMesh && Projectile->ProjectileMesh)
		{
			Projectile->ProjectileMesh->SetStaticMesh(ProjectileMesh->GetStaticMesh());
		}

		if (TrailEffect && Projectile->TrailEffect)
		{
			Projectile->TrailEffect->SetAsset(TrailEffect->GetAsset());
		}

		if (HitEffect && Projectile->HitEffect)
		{
			Projectile->HitEffect->SetAsset(HitEffect->GetAsset());
		}

		Projectile->HitSound = HitSound;
	}

	if (FireSound)
	{
		UGameplayStatics::PlaySoundAtLocation(TargetCar, FireSound, SpawnLocation);
	}

	SetLifeSpan(1.0f);
}
