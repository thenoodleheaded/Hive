// Copyright Epic Games, Inc. All Rights Reserved.

#include "PowerUps/PowerUpTestStation.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "Hive.h"
#include "HiveSportsCar.h"
#include "Kismet/GameplayStatics.h"
#include "PowerUps/NavigationRocket.h"
#include "PowerUps/PowerUpBase.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

APowerUpTestStation::APowerUpTestStation()
{
	PrimaryActorTick.bCanEverTick = false;

	StationMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StationMesh"));
	SetRootComponent(StationMesh);
	StationMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	StationMesh->SetCanEverAffectNavigation(false);
	StationMesh->SetCastShadow(false);
	StationMesh->SetRelativeScale3D(FVector(1.0f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMeshFinder.Succeeded())
	{
		StationMesh->SetStaticMesh(CubeMeshFinder.Object);
	}
}

void APowerUpTestStation::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogHive, Verbose, TEXT("[PowerUpTestStation] BeginPlay Station=%s Location=%s Forward=%s PowerUpClass=%s bAutoFire=%s FireInterval=%.2f OverrideRocket=%s"),
		*GetNameSafe(this),
		*GetActorLocation().ToCompactString(),
		*GetActorForwardVector().ToCompactString(),
		*GetNameSafe(PowerUpClassToFire.Get()),
		bAutoFire ? TEXT("true") : TEXT("false"),
		FireInterval,
		bOverrideNavigationRocketTuning ? TEXT("true") : TEXT("false"));

	if (bAutoFire && FireInterval > 0.0f)
	{
		GetWorldTimerManager().SetTimer(AutoFireTimerHandle, this, &APowerUpTestStation::FireNow, FireInterval, true, FireInterval);
		UE_LOG(LogHive, Verbose, TEXT("[PowerUpTestStation] Auto-fire timer armed for %s every %.2fs."),
			*GetNameSafe(this),
			FireInterval);
	}
}

void APowerUpTestStation::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AutoFireTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

void APowerUpTestStation::FireNow()
{
	if (!PowerUpClassToFire || !GetWorld())
	{
		UE_LOG(LogHive, Warning, TEXT("[PowerUpTestStation] FireNow aborted on %s. PowerUpClass=%s WorldValid=%s"),
			*GetNameSafe(this),
			*GetNameSafe(PowerUpClassToFire.Get()),
			GetWorld() ? TEXT("true") : TEXT("false"));
		return;
	}

	UE_LOG(LogHive, Verbose, TEXT("[PowerUpTestStation] FireNow Station=%s Class=%s Location=%s Forward=%s"),
		*GetNameSafe(this),
		*GetNameSafe(PowerUpClassToFire.Get()),
		*GetActorLocation().ToCompactString(),
		*GetActorForwardVector().ToCompactString());

	AHiveSportsCar* TargetCar = FindTargetCar();
	if (!TargetCar)
	{
		UE_LOG(LogHive, Warning, TEXT("[PowerUpTestStation] %s could not find an AHiveSportsCar target for %s."),
			*GetNameSafe(this),
			*GetNameSafe(PowerUpClassToFire.Get()));
		return;
	}

	UE_LOG(LogHive, Verbose, TEXT("[PowerUpTestStation] Target selected Station=%s Target=%s TargetLocation=%s Distance=%.1f"),
		*GetNameSafe(this),
		*GetNameSafe(TargetCar),
		*TargetCar->GetActorLocation().ToCompactString(),
		FVector::Dist(GetActorLocation(), TargetCar->GetActorLocation()));

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	APowerUpBase* PowerUp = GetWorld()->SpawnActor<APowerUpBase>(PowerUpClassToFire, GetActorLocation(), GetActorRotation(), SpawnParameters);
	if (!PowerUp)
	{
		UE_LOG(LogHive, Warning, TEXT("[PowerUpTestStation] %s failed to spawn %s for %s."),
			*GetNameSafe(this),
			*GetNameSafe(PowerUpClassToFire.Get()),
			*GetNameSafe(TargetCar));
		return;
	}

	UE_LOG(LogHive, Verbose, TEXT("[PowerUpTestStation] Spawned power-up instance %s at %s Rotation=%s Owner=%s"),
		*GetNameSafe(PowerUp),
		*PowerUp->GetActorLocation().ToCompactString(),
		*PowerUp->GetActorRotation().ToCompactString(),
		*GetNameSafe(PowerUp->GetOwner()));

	if (bOverrideNavigationRocketTuning)
	{
		if (ANavigationRocket* NavigationRocket = Cast<ANavigationRocket>(PowerUp))
		{
			NavigationRocket->RocketSpeed = NavigationRocketSpeed;
			NavigationRocket->HomingAcceleration = NavigationRocketHomingAcceleration;
			NavigationRocket->RocketLifetime = NavigationRocketLifetime;
			NavigationRocket->ImpactImpulse = NavigationRocketImpactImpulse;
			NavigationRocket->DriftKickRearOffset = NavigationRocketDriftKickRearOffset;
			NavigationRocket->DriftKickDownforceImpulse = NavigationRocketDriftKickDownforceImpulse;
			NavigationRocket->MaxUpwardVelocityAfterHit = NavigationRocketMaxUpwardVelocityAfterHit;
			UE_LOG(LogHive, Verbose, TEXT("[PowerUpTestStation] Applied NavigationRocket override Speed=%.1f Homing=%.1f Lifetime=%.1f Impact=%.1f RearOffset=%.1f Downforce=%.1f MaxUpZ=%.1f"),
				NavigationRocketSpeed,
				NavigationRocketHomingAcceleration,
				NavigationRocketLifetime,
				NavigationRocketImpactImpulse,
				NavigationRocketDriftKickRearOffset,
				NavigationRocketDriftKickDownforceImpulse,
				NavigationRocketMaxUpwardVelocityAfterHit);
		}
	}

	PowerUp->SetActivationContext(GetActorTransform(), nullptr, TargetCar, this);
	UE_LOG(LogHive, Verbose, TEXT("[PowerUpTestStation] Activating %s with StationSource=%s Target=%s."),
		*GetNameSafe(PowerUp),
		*GetNameSafe(this),
		*GetNameSafe(TargetCar));
	PowerUp->OnPickup(TargetCar);
	PowerUp->OnActivate(TargetCar);

	UE_LOG(LogHive, Verbose, TEXT("[PowerUpTestStation] %s fired %s at %s."),
		*GetNameSafe(this),
		*GetNameSafe(PowerUpClassToFire.Get()),
		*GetNameSafe(TargetCar));
}

AHiveSportsCar* APowerUpTestStation::FindTargetCar() const
{
	if (APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		if (AHiveSportsCar* PlayerSportsCar = Cast<AHiveSportsCar>(PlayerPawn))
		{
			UE_LOG(LogHive, Verbose, TEXT("[PowerUpTestStation] FindTargetCar using player pawn %s."),
				*GetNameSafe(PlayerSportsCar));
			return PlayerSportsCar;
		}

		UE_LOG(LogHive, Verbose, TEXT("[PowerUpTestStation] Player pawn %s is not AHiveSportsCar; searching world."),
			*GetNameSafe(PlayerPawn));
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	AHiveSportsCar* BestCar = nullptr;
	float BestDistanceSquared = TNumericLimits<float>::Max();

	for (TActorIterator<AHiveSportsCar> It(World); It; ++It)
	{
		AHiveSportsCar* Candidate = *It;
		if (!Candidate)
		{
			continue;
		}

		const float DistanceSquared = FVector::DistSquared(GetActorLocation(), Candidate->GetActorLocation());
		if (DistanceSquared < BestDistanceSquared)
		{
			BestDistanceSquared = DistanceSquared;
			BestCar = Candidate;
		}
	}

	UE_LOG(LogHive, Verbose, TEXT("[PowerUpTestStation] FindTargetCar nearest result=%s Distance=%.1f"),
		*GetNameSafe(BestCar),
		BestCar ? FMath::Sqrt(BestDistanceSquared) : -1.0f);

	return BestCar;
}
