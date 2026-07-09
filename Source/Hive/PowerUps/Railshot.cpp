// Copyright Epic Games, Inc. All Rights Reserved.

#include "PowerUps/Railshot.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Hive.h"
#include "HiveSportsCar.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "PowerUps/PowerUpImpactEffects.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr float RailshotSpawnForwardOffset = 300.0f;
	constexpr float RailshotSpawnHeightOffset = 75.0f;
	constexpr float RailshotHitGracePeriod = 0.1f;
	constexpr float RailshotSteeringReductionMultiplier = 0.3f;
}

ARailshotProjectile::ARailshotProjectile()
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
	ProjectileMesh->SetRelativeScale3D(FVector(0.12f, 0.12f, 1.1f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMeshFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMeshFinder.Succeeded())
	{
		ProjectileMesh->SetStaticMesh(CylinderMeshFinder.Object);
	}

	TrailEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("TrailEffect"));
	TrailEffect->SetupAttachment(CollisionSphere);
	TrailEffect->bAutoActivate = false;

	ExplosionEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("ExplosionEffect"));
	ExplosionEffect->SetupAttachment(CollisionSphere);
	ExplosionEffect->bAutoActivate = false;

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->InitialSpeed = ProjectileSpeed;
	ProjectileMovement->MaxSpeed = ProjectileSpeed;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = false;
	ProjectileMovement->ProjectileGravityScale = 0.0f;
	ProjectileMovement->bIsHomingProjectile = false;
}

void ARailshotProjectile::Initialize(AHiveSportsCar* InSourceCar, float InProjectileSpeed, float InProjectileLifetime, float InProjectileRadius, float InExplosionRadius, float InImpactImpulse, float InDriftKickRearOffset, float InDriftKickDownforceImpulse, float InMaxUpwardVelocityAfterHit, float InSteeringReductionDuration, const FVector& FireDirection)
{
	SourceCar = InSourceCar;
	ProjectileSpeed = InProjectileSpeed;
	ProjectileLifetime = InProjectileLifetime;
	ProjectileRadius = InProjectileRadius;
	ExplosionRadius = InExplosionRadius;
	ImpactImpulse = InImpactImpulse;
	DriftKickRearOffset = InDriftKickRearOffset;
	DriftKickDownforceImpulse = InDriftKickDownforceImpulse;
	MaxUpwardVelocityAfterHit = InMaxUpwardVelocityAfterHit;
	SteeringReductionDuration = InSteeringReductionDuration;

	if (CollisionSphere)
	{
		CollisionSphere->SetSphereRadius(ProjectileRadius);
		if (InSourceCar)
		{
			CollisionSphere->IgnoreActorWhenMoving(InSourceCar, true);
		}
	}

	if (!ProjectileMovement)
	{
		return;
	}

	const FVector LaunchDirection = FireDirection.GetSafeNormal();
	ProjectileMovement->InitialSpeed = ProjectileSpeed;
	ProjectileMovement->MaxSpeed = ProjectileSpeed;
	ProjectileMovement->Velocity = LaunchDirection * ProjectileSpeed;
	ProjectileMovement->bIsHomingProjectile = false;
	ProjectileMovement->HomingTargetComponent = nullptr;

	UE_LOG(LogHive, Verbose, TEXT("[Railshot] Initialize straight shot: Projectile=%s Source=%s Location=%s Direction=%s InitialVelocity=%s Speed=%.1f Lifetime=%.2f Radius=%.1f ExplosionRadius=%.1f Impact=%.1f"),
		*GetNameSafe(this),
		*GetNameSafe(InSourceCar),
		*GetActorLocation().ToCompactString(),
		*LaunchDirection.ToCompactString(),
		*ProjectileMovement->Velocity.ToCompactString(),
		ProjectileSpeed,
		ProjectileLifetime,
		ProjectileRadius,
		ExplosionRadius,
		ImpactImpulse);

	ProjectileMovement->Activate(true);
}

void ARailshotProjectile::BeginPlay()
{
	Super::BeginPlay();

	if (CollisionSphere)
	{
		CollisionSphere->OnComponentHit.AddDynamic(this, &ARailshotProjectile::OnRailshotHit);
		CollisionSphere->SetSphereRadius(ProjectileRadius);
	}

	UE_LOG(LogHive, Verbose, TEXT("[Railshot] BeginPlay Projectile=%s Location=%s Radius=%.1f Lifetime=%.2f Collision=%s"),
		*GetNameSafe(this),
		*GetActorLocation().ToCompactString(),
		ProjectileRadius,
		ProjectileLifetime,
		CollisionSphere ? *UEnum::GetValueAsString(CollisionSphere->GetCollisionEnabled()) : TEXT("NoCollisionSphere"));

	if (TrailEffect)
	{
		TrailEffect->Activate(true);
	}

	GetWorldTimerManager().SetTimer(LifetimeTimerHandle, this, &ARailshotProjectile::FinishProjectileLifetime, ProjectileLifetime, false);
	GetWorldTimerManager().SetTimer(HitGraceTimerHandle, this, &ARailshotProjectile::EnableHitRegistration, RailshotHitGracePeriod, false);
}

void ARailshotProjectile::OnRailshotHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	const float CurrentProjectileSpeed = ProjectileMovement ? ProjectileMovement->Velocity.Size() : GetVelocity().Size();
	UE_LOG(LogHive, Verbose, TEXT("[Railshot] OnRailshotHit immediate: Projectile=%s OtherActor=%s OtherComp=%s ProjectileSpeed=%.1f HitLocation=%s ImpactPoint=%s bCanRegisterHit=%s"),
		*GetNameSafe(this),
		*GetNameSafe(OtherActor),
		*GetNameSafe(OtherComp),
		CurrentProjectileSpeed,
		*GetActorLocation().ToCompactString(),
		*Hit.ImpactPoint.ToCompactString(),
		bCanRegisterHit ? TEXT("true") : TEXT("false"));

	if (!bCanRegisterHit)
	{
		UE_LOG(LogHive, Verbose, TEXT("[Railshot] Ignored early hit during %.2fs grace period. OtherActor=%s"),
			RailshotHitGracePeriod,
			*GetNameSafe(OtherActor));
		return;
	}

	if (!OtherActor || OtherActor == this)
	{
		return;
	}

	AHiveSportsCar* HitCar = Cast<AHiveSportsCar>(OtherActor);
	if (HitCar && HitCar == SourceCar.Get())
	{
		UE_LOG(LogHive, Verbose, TEXT("[Railshot] Ignored source-car hit. HitCar=%s SourceCar=%s"),
			*GetNameSafe(HitCar),
			*GetNameSafe(SourceCar.Get()));
		return;
	}

	if (HitCar)
	{
		AHiveSportsCar* PreviousSourceCar = SourceCar.Get();
		if (HitCar->TryReflectProjectile(this))
		{
			if (CollisionSphere && PreviousSourceCar)
			{
				CollisionSphere->IgnoreActorWhenMoving(PreviousSourceCar, false);
			}

			UE_LOG(LogHive, Verbose, TEXT("[Railshot] Reflected by mirror shield on %s."), *GetNameSafe(HitCar));
			SourceCar = HitCar;
			return;
		}

		if (HitCar->TryAbsorbHit())
		{
			UE_LOG(LogHive, Verbose, TEXT("[Railshot] Hit absorbed by shield on %s."), *GetNameSafe(HitCar));
			Destroy();
			return;
		}
	}

	const FVector ImpactLocation = Hit.ImpactPoint.IsNearlyZero() ? GetActorLocation() : FVector(Hit.ImpactPoint);
	Explode(ImpactLocation, HitCar);
}

void ARailshotProjectile::Explode(const FVector& HitLocation, AHiveSportsCar* DirectHitCar)
{
	UE_LOG(LogHive, Verbose, TEXT("[Railshot] Explode at %s with ImpactImpulse=%.1f ExplosionRadius=%.1f"),
		*HitLocation.ToCompactString(),
		ImpactImpulse,
		ExplosionRadius);

	PlayImpactEffects(HitLocation);

	FHiveProjectileImpactParams ImpactParams;
	ImpactParams.ExplosionRadius = ExplosionRadius;
	ImpactParams.ImpactImpulse = ImpactImpulse;
	ImpactParams.DriftKickRearOffset = DriftKickRearOffset;
	ImpactParams.DriftKickDownforceImpulse = DriftKickDownforceImpulse;
	ImpactParams.MaxUpwardVelocityAfterHit = MaxUpwardVelocityAfterHit;
	ImpactParams.SteeringReductionDuration = SteeringReductionDuration;
	ImpactParams.SteeringReductionMultiplier = RailshotSteeringReductionMultiplier;

	const FVector CurrentProjectileVelocity = ProjectileMovement ? ProjectileMovement->Velocity : GetVelocity();
	HivePowerUpImpactEffects::ApplyProjectileImpact(this, HitLocation, CurrentProjectileVelocity, DirectHitCar, ImpactParams, TEXT("Railshot"));

	HideProjectileAfterImpact();
	Destroy();
}

void ARailshotProjectile::PlayImpactEffects(const FVector& HitLocation)
{
	if (TrailEffect)
	{
		TrailEffect->Deactivate();
	}

	if (ExplosionEffect && ExplosionEffect->GetAsset())
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, ExplosionEffect->GetAsset(), HitLocation);
	}

	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, HitLocation);
	}
}

void ARailshotProjectile::FinishProjectileLifetime()
{
	Destroy();
}

void ARailshotProjectile::HideProjectileAfterImpact()
{
	GetWorldTimerManager().ClearTimer(LifetimeTimerHandle);
	GetWorldTimerManager().ClearTimer(HitGraceTimerHandle);

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

void ARailshotProjectile::EnableHitRegistration()
{
	bCanRegisterHit = true;

	if (CollisionSphere)
	{
		CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		CollisionSphere->SetNotifyRigidBodyCollision(true);
	}

	UE_LOG(LogHive, Verbose, TEXT("[Railshot] Hit registration enabled after %.2fs grace period at %s Velocity=%s Collision=%s"),
		RailshotHitGracePeriod,
		*GetActorLocation().ToCompactString(),
		ProjectileMovement ? *ProjectileMovement->Velocity.ToCompactString() : TEXT("NoProjectileMovement"),
		CollisionSphere ? *UEnum::GetValueAsString(CollisionSphere->GetCollisionEnabled()) : TEXT("NoCollisionSphere"));
}

ARailshot::ARailshot()
{
	Category = EPowerUpCategory::Projectile;
	DisplayName = FText::FromString(TEXT("Railshot"));
	Duration = 0.0f;
}

void ARailshot::OnActivate_Implementation(AHiveSportsCar* InstigatorCar)
{
	AHiveSportsCar* TargetCar = GetActivationTargetCar(InstigatorCar ? InstigatorCar : OwningCar);
	if (!TargetCar || !GetWorld())
	{
		return;
	}

	AHiveSportsCar* SourceCar = GetActivationSourceCar(TargetCar);
	AActor* SourceActor = GetActivationSourceActor(SourceCar ? Cast<AActor>(SourceCar) : Cast<AActor>(TargetCar));
	AHiveSportsCar* OwnerCar = SourceCar ? SourceCar : TargetCar;
	const FVector FireDirection = GetActivationSourceForwardVector(TargetCar).GetSafeNormal();
	const FVector SpawnLocation = GetActivationSourceLocation(TargetCar) + FireDirection * RailshotSpawnForwardOffset + FVector::UpVector * RailshotSpawnHeightOffset;
	const FRotator SpawnRotation = FireDirection.Rotation();

	UE_LOG(LogHive, Verbose, TEXT("[Railshot] Activate SourceActor=%s SourceCar=%s Target=%s SourceLocation=%s SourceForward=%s SpawnLocation=%s SpawnRotation=%s"),
		*GetNameSafe(SourceActor),
		*GetNameSafe(SourceCar),
		*GetNameSafe(TargetCar),
		*GetActivationSourceLocation(TargetCar).ToCompactString(),
		*FireDirection.ToCompactString(),
		*SpawnLocation.ToCompactString(),
		*SpawnRotation.ToCompactString());

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = OwnerCar;
	SpawnParameters.Instigator = OwnerCar;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ARailshotProjectile* Projectile = GetWorld()->SpawnActor<ARailshotProjectile>(ARailshotProjectile::StaticClass(), SpawnLocation, SpawnRotation, SpawnParameters);
	if (Projectile)
	{
		UE_LOG(LogHive, Verbose, TEXT("[Railshot] Spawned from Source=%s at %s ForwardOffset=%.1f HeightOffset=%.1f Direction=%s Target=%s OwnerCar=%s"),
			*GetNameSafe(SourceActor),
			*SpawnLocation.ToCompactString(),
			RailshotSpawnForwardOffset,
			RailshotSpawnHeightOffset,
			*FireDirection.ToCompactString(),
			*GetNameSafe(TargetCar),
			*GetNameSafe(OwnerCar));

		Projectile->Initialize(SourceCar, ProjectileSpeed, ProjectileLifetime, ProjectileRadius, ExplosionRadius, ImpactImpulse, DriftKickRearOffset, DriftKickDownforceImpulse, MaxUpwardVelocityAfterHit, SteeringReductionDuration, FireDirection);

		if (ProjectileMesh && Projectile->ProjectileMesh)
		{
			Projectile->ProjectileMesh->SetStaticMesh(ProjectileMesh->GetStaticMesh());
		}

		if (TrailEffect && Projectile->TrailEffect)
		{
			Projectile->TrailEffect->SetAsset(TrailEffect->GetAsset());
		}

		if (ExplosionEffect && Projectile->ExplosionEffect)
		{
			Projectile->ExplosionEffect->SetAsset(ExplosionEffect->GetAsset());
		}

		Projectile->ImpactSound = ImpactSound;
	}

	if (FireSound)
	{
		UGameplayStatics::PlaySoundAtLocation(OwnerCar, FireSound, SpawnLocation);
	}

	SetLifeSpan(1.0f);
}
