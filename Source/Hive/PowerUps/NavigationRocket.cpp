// Copyright Epic Games, Inc. All Rights Reserved.

#include "PowerUps/NavigationRocket.h"
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
	constexpr float RocketSpawnForwardOffset = 300.0f;
	constexpr float RocketSpawnHeightOffset = 75.0f;
	constexpr float RocketHitGracePeriod = 0.1f;
	constexpr float RocketSteeringReductionMultiplier = 0.3f;
}

ANavigationRocketProjectile::ANavigationRocketProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	SetRootComponent(CollisionSphere);
	CollisionSphere->InitSphereRadius(40.0f);
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CollisionSphere->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionSphere->SetCollisionResponseToAllChannels(ECR_Block);
	CollisionSphere->SetGenerateOverlapEvents(false);
	CollisionSphere->SetNotifyRigidBodyCollision(false);
	CollisionSphere->SetCanEverAffectNavigation(false);

	RocketMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RocketMesh"));
	RocketMesh->SetupAttachment(CollisionSphere);
	RocketMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RocketMesh->SetCanEverAffectNavigation(false);
	RocketMesh->SetCastShadow(false);
	RocketMesh->SetRelativeScale3D(FVector(0.25f, 0.25f, 0.8f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMeshFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMeshFinder.Succeeded())
	{
		RocketMesh->SetStaticMesh(CylinderMeshFinder.Object);
	}

	TrailEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("TrailEffect"));
	TrailEffect->SetupAttachment(CollisionSphere);
	TrailEffect->bAutoActivate = false;

	ExplosionEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("ExplosionEffect"));
	ExplosionEffect->SetupAttachment(CollisionSphere);
	ExplosionEffect->bAutoActivate = false;

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->InitialSpeed = RocketSpeed;
	ProjectileMovement->MaxSpeed = RocketSpeed;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = false;
	ProjectileMovement->ProjectileGravityScale = 0.0f;
}

void ANavigationRocketProjectile::Initialize(AHiveSportsCar* InSourceCar, AHiveSportsCar* InTargetCar, float InRocketSpeed, float InHomingAcceleration, float InRocketLifetime, float InExplosionRadius, float InImpactImpulse, float InDriftKickRearOffset, float InDriftKickDownforceImpulse, float InMaxUpwardVelocityAfterHit, float InSteeringReductionDuration, const FVector& FireDirection)
{
	SourceCar = InSourceCar;
	RocketSpeed = InRocketSpeed;
	HomingAcceleration = InHomingAcceleration;
	RocketLifetime = InRocketLifetime;
	ExplosionRadius = InExplosionRadius;
	ImpactImpulse = InImpactImpulse;
	DriftKickRearOffset = InDriftKickRearOffset;
	DriftKickDownforceImpulse = InDriftKickDownforceImpulse;
	MaxUpwardVelocityAfterHit = InMaxUpwardVelocityAfterHit;
	SteeringReductionDuration = InSteeringReductionDuration;

	if (CollisionSphere && InSourceCar)
	{
		CollisionSphere->IgnoreActorWhenMoving(InSourceCar, true);
	}

	if (!ProjectileMovement)
	{
		return;
	}

	const FVector LaunchDirection = FireDirection.GetSafeNormal();
	ProjectileMovement->InitialSpeed = RocketSpeed;
	ProjectileMovement->MaxSpeed = RocketSpeed;
	ProjectileMovement->Velocity = LaunchDirection * RocketSpeed;
	ProjectileMovement->HomingAccelerationMagnitude = HomingAcceleration;

	if (InTargetCar)
	{
		ProjectileMovement->bIsHomingProjectile = true;
		ProjectileMovement->HomingTargetComponent = InTargetCar->GetRootComponent();
	}
	else
	{
		ProjectileMovement->bIsHomingProjectile = false;
		ProjectileMovement->HomingTargetComponent = nullptr;
	}

	UE_LOG(LogHive, Verbose, TEXT("[NavigationRocket] Initialize homing: Projectile=%s Source=%s Target=%s Location=%s Direction=%s bIsHoming=%s HomingTargetComponent=%s InitialVelocity=%s Speed=%.1f HomingAcceleration=%.1f Lifetime=%.2f ExplosionRadius=%.1f Impact=%.1f"),
		*GetNameSafe(this),
		*GetNameSafe(InSourceCar),
		*GetNameSafe(InTargetCar),
		*GetActorLocation().ToCompactString(),
		*LaunchDirection.ToCompactString(),
		ProjectileMovement->bIsHomingProjectile ? TEXT("true") : TEXT("false"),
		*GetNameSafe(ProjectileMovement->HomingTargetComponent.Get()),
		*ProjectileMovement->Velocity.ToCompactString(),
		RocketSpeed,
		ProjectileMovement->HomingAccelerationMagnitude,
		RocketLifetime,
		ExplosionRadius,
		ImpactImpulse);

	ProjectileMovement->Activate(true);
}

void ANavigationRocketProjectile::BeginPlay()
{
	Super::BeginPlay();

	if (CollisionSphere)
	{
		CollisionSphere->OnComponentHit.AddDynamic(this, &ANavigationRocketProjectile::OnRocketHit);
	}

	UE_LOG(LogHive, Verbose, TEXT("[NavigationRocket] BeginPlay Projectile=%s Location=%s Radius=%.1f Lifetime=%.2f Collision=%s"),
		*GetNameSafe(this),
		*GetActorLocation().ToCompactString(),
		CollisionSphere ? CollisionSphere->GetScaledSphereRadius() : 0.0f,
		RocketLifetime,
		CollisionSphere ? *UEnum::GetValueAsString(CollisionSphere->GetCollisionEnabled()) : TEXT("NoCollisionSphere"));

	if (TrailEffect)
	{
		TrailEffect->Activate(true);
	}

	GetWorldTimerManager().SetTimer(LifetimeTimerHandle, this, &ANavigationRocketProjectile::FinishRocketLifetime, RocketLifetime, false);
	GetWorldTimerManager().SetTimer(HitGraceTimerHandle, this, &ANavigationRocketProjectile::EnableHitRegistration, RocketHitGracePeriod, false);
}

void ANavigationRocketProjectile::OnRocketHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	const float CurrentRocketSpeed = ProjectileMovement ? ProjectileMovement->Velocity.Size() : GetVelocity().Size();
	UE_LOG(LogHive, Verbose, TEXT("[NavigationRocket] OnRocketHit immediate: Projectile=%s OtherActor=%s OtherComp=%s RocketSpeed=%.1f HitLocation=%s ImpactPoint=%s bCanRegisterHit=%s"),
		*GetNameSafe(this),
		*GetNameSafe(OtherActor),
		*GetNameSafe(OtherComp),
		CurrentRocketSpeed,
		*GetActorLocation().ToCompactString(),
		*Hit.ImpactPoint.ToCompactString(),
		bCanRegisterHit ? TEXT("true") : TEXT("false"));

	if (!bCanRegisterHit)
	{
		UE_LOG(LogHive, Verbose, TEXT("[NavigationRocket] Ignored early hit during %.2fs grace period. OtherActor=%s"),
			RocketHitGracePeriod,
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
		UE_LOG(LogHive, Verbose, TEXT("[NavigationRocket] Ignored source-car hit. HitCar=%s SourceCar=%s"),
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

			SourceCar = HitCar;
			if (ProjectileMovement)
			{
				ProjectileMovement->bIsHomingProjectile = IsValid(PreviousSourceCar);
				ProjectileMovement->HomingTargetComponent = PreviousSourceCar ? PreviousSourceCar->GetRootComponent() : nullptr;
			}

			UE_LOG(LogHive, Verbose, TEXT("[NavigationRocket] Reflected by mirror shield on %s. NewSource=%s NewHomingTarget=%s"),
				*GetNameSafe(HitCar),
				*GetNameSafe(SourceCar.Get()),
				ProjectileMovement ? *GetNameSafe(ProjectileMovement->HomingTargetComponent.Get()) : TEXT("NoProjectileMovement"));
			return;
		}

		if (HitCar->TryAbsorbHit())
		{
			UE_LOG(LogHive, Verbose, TEXT("[NavigationRocket] Hit absorbed by shield on %s."), *GetNameSafe(HitCar));
			Destroy();
			return;
		}
	}

	const FVector ImpactLocation = Hit.ImpactPoint.IsNearlyZero() ? GetActorLocation() : FVector(Hit.ImpactPoint);
	Explode(ImpactLocation, HitCar);
}

void ANavigationRocketProjectile::Explode(const FVector& HitLocation, AHiveSportsCar* DirectHitCar)
{
	UE_LOG(LogHive, Verbose, TEXT("[NavigationRocket] Explode at %s with ImpactImpulse=%.1f ExplosionRadius=%.1f"),
		*HitLocation.ToCompactString(),
		ImpactImpulse,
		ExplosionRadius);

	PlayExplosionEffects(HitLocation);
	FHiveProjectileImpactParams ImpactParams;
	ImpactParams.ExplosionRadius = ExplosionRadius;
	ImpactParams.ImpactImpulse = ImpactImpulse;
	ImpactParams.DriftKickRearOffset = DriftKickRearOffset;
	ImpactParams.DriftKickDownforceImpulse = DriftKickDownforceImpulse;
	ImpactParams.MaxUpwardVelocityAfterHit = MaxUpwardVelocityAfterHit;
	ImpactParams.SteeringReductionDuration = SteeringReductionDuration;
	ImpactParams.SteeringReductionMultiplier = RocketSteeringReductionMultiplier;

	const FVector CurrentProjectileVelocity = ProjectileMovement ? ProjectileMovement->Velocity : GetVelocity();
	HivePowerUpImpactEffects::ApplyProjectileImpact(this, HitLocation, CurrentProjectileVelocity, DirectHitCar, ImpactParams, TEXT("NavigationRocket"));
	HideRocketAfterImpact();
	Destroy();
}

void ANavigationRocketProjectile::PlayExplosionEffects(const FVector& HitLocation)
{
	if (TrailEffect)
	{
		TrailEffect->Deactivate();
	}

	if (ExplosionEffect && ExplosionEffect->GetAsset())
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, ExplosionEffect->GetAsset(), HitLocation);
	}

	if (ExplosionSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ExplosionSound, HitLocation);
	}
}

void ANavigationRocketProjectile::FinishRocketLifetime()
{
	Destroy();
}

void ANavigationRocketProjectile::HideRocketAfterImpact()
{
	GetWorldTimerManager().ClearTimer(LifetimeTimerHandle);
	GetWorldTimerManager().ClearTimer(HitGraceTimerHandle);

	if (CollisionSphere)
	{
		CollisionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (RocketMesh)
	{
		RocketMesh->SetVisibility(false, true);
	}

	if (ProjectileMovement)
	{
		ProjectileMovement->StopMovementImmediately();
		ProjectileMovement->Deactivate();
	}
}

void ANavigationRocketProjectile::EnableHitRegistration()
{
	bCanRegisterHit = true;

	if (CollisionSphere)
	{
		CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		CollisionSphere->SetNotifyRigidBodyCollision(true);
	}

	UE_LOG(LogHive, Verbose, TEXT("[NavigationRocket] Hit registration enabled after %.2fs grace period at %s Velocity=%s Collision=%s"),
		RocketHitGracePeriod,
		*GetActorLocation().ToCompactString(),
		ProjectileMovement ? *ProjectileMovement->Velocity.ToCompactString() : TEXT("NoProjectileMovement"),
		CollisionSphere ? *UEnum::GetValueAsString(CollisionSphere->GetCollisionEnabled()) : TEXT("NoCollisionSphere"));
}

ANavigationRocket::ANavigationRocket()
{
	Category = EPowerUpCategory::Projectile;
	DisplayName = FText::FromString(TEXT("Navigation Rocket"));
	Duration = 0.0f;
}

void ANavigationRocket::OnActivate_Implementation(AHiveSportsCar* InstigatorCar)
{
	AHiveSportsCar* TargetCar = GetActivationTargetCar(InstigatorCar ? InstigatorCar : OwningCar);
	if (!TargetCar || !GetWorld())
	{
		return;
	}

	AHiveSportsCar* SourceCar = GetActivationSourceCar(TargetCar);
	AActor* SourceActor = GetActivationSourceActor(SourceCar ? Cast<AActor>(SourceCar) : Cast<AActor>(TargetCar));
	const FVector FireDirection = GetActivationSourceForwardVector(TargetCar);
	const FVector SpawnLocation = GetActivationSourceLocation(TargetCar) + FireDirection * RocketSpawnForwardOffset + FVector::UpVector * RocketSpawnHeightOffset;
	const FRotator SpawnRotation = FireDirection.Rotation();

	UE_LOG(LogHive, Verbose, TEXT("[NavigationRocket] Activate SourceActor=%s SourceCar=%s Target=%s SourceLocation=%s SourceForward=%s SpawnLocation=%s SpawnRotation=%s"),
		*GetNameSafe(SourceActor),
		*GetNameSafe(SourceCar),
		*GetNameSafe(TargetCar),
		*GetActivationSourceLocation(TargetCar).ToCompactString(),
		*FireDirection.ToCompactString(),
		*SpawnLocation.ToCompactString(),
		*SpawnRotation.ToCompactString());

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = SourceCar ? SourceCar : TargetCar;
	SpawnParameters.Instigator = SourceCar ? SourceCar : TargetCar;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ANavigationRocketProjectile* Rocket = GetWorld()->SpawnActor<ANavigationRocketProjectile>(ANavigationRocketProjectile::StaticClass(), SpawnLocation, SpawnRotation, SpawnParameters);
	if (Rocket)
	{
		UE_LOG(LogHive, Verbose, TEXT("[NavigationRocket] Spawned from Source=%s at %s ForwardOffset=%.1f HeightOffset=%.1f Target=%s OwnerCar=%s"),
			*GetNameSafe(SourceActor),
			*SpawnLocation.ToCompactString(),
			RocketSpawnForwardOffset,
			RocketSpawnHeightOffset,
			*GetNameSafe(TargetCar),
			*GetNameSafe(SourceCar ? SourceCar : TargetCar));

		Rocket->Initialize(SourceCar, TargetCar, RocketSpeed, HomingAcceleration, RocketLifetime, ExplosionRadius, ImpactImpulse, DriftKickRearOffset, DriftKickDownforceImpulse, MaxUpwardVelocityAfterHit, SteeringReductionDuration, FireDirection);

		if (RocketMesh && Rocket->RocketMesh)
		{
			Rocket->RocketMesh->SetStaticMesh(RocketMesh->GetStaticMesh());
		}

		if (TrailEffect && Rocket->TrailEffect)
		{
			Rocket->TrailEffect->SetAsset(TrailEffect->GetAsset());
		}

		if (ExplosionEffect && Rocket->ExplosionEffect)
		{
			Rocket->ExplosionEffect->SetAsset(ExplosionEffect->GetAsset());
		}

		Rocket->ExplosionSound = ExplosionSound;
	}

	if (LaunchSound)
	{
		UGameplayStatics::PlaySoundAtLocation(TargetCar, LaunchSound, SpawnLocation);
	}

	SetLifeSpan(1.0f);
}
