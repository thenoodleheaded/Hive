// Copyright Epic Games, Inc. All Rights Reserved.

#include "PowerUps/OilSlick.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "HiveSportsCar.h"
#include "HiveVehicleMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

ASlickZoneActor::ASlickZoneActor()
{
	PrimaryActorTick.bCanEverTick = false;

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	SetRootComponent(CollisionSphere);
	CollisionSphere->InitSphereRadius(SlickRadius);
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionSphere->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CollisionSphere->SetCollisionResponseToChannel(ECC_Vehicle, ECR_Overlap);
	CollisionSphere->SetGenerateOverlapEvents(true);
	CollisionSphere->SetCanEverAffectNavigation(false);

	SlickMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SlickMesh"));
	SlickMesh->SetupAttachment(CollisionSphere);
	SlickMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SlickMesh->SetCanEverAffectNavigation(false);
	SlickMesh->SetCastShadow(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMeshFinder(TEXT("/Engine/BasicShapes/Plane.Plane"));
	if (PlaneMeshFinder.Succeeded())
	{
		SlickMesh->SetStaticMesh(PlaneMeshFinder.Object);
	}
}

void ASlickZoneActor::Initialize(float InSlickRadius, float InGripReduction, float InSlickDuration, float InEffectDuration)
{
	SlickRadius = InSlickRadius;
	GripReduction = InGripReduction;
	SlickDuration = InSlickDuration;
	EffectDuration = InEffectDuration;

	if (CollisionSphere)
	{
		CollisionSphere->SetSphereRadius(SlickRadius);
	}

	UpdateZoneVisualScale();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SlickLifetimeTimerHandle);
		World->GetTimerManager().SetTimer(SlickLifetimeTimerHandle, this, &ASlickZoneActor::DestroySlickZone, SlickDuration, false);
	}
}

void ASlickZoneActor::BeginPlay()
{
	Super::BeginPlay();

	if (CollisionSphere)
	{
		CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &ASlickZoneActor::OnSlickOverlap);
		CollisionSphere->SetSphereRadius(SlickRadius);
	}

	UpdateZoneVisualScale();

	if (SlickEffect)
	{
		SlickEffect->SetVisibility(true, true);
		SlickEffect->Activate(true);
	}

	if (!GetWorldTimerManager().IsTimerActive(SlickLifetimeTimerHandle))
	{
		GetWorldTimerManager().SetTimer(SlickLifetimeTimerHandle, this, &ASlickZoneActor::DestroySlickZone, SlickDuration, false);
	}
}

void ASlickZoneActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (CollisionSphere)
	{
		CollisionSphere->SetSphereRadius(SlickRadius);
	}

	UpdateZoneVisualScale();
}

void ASlickZoneActor::Destroyed()
{
	TArray<TWeakObjectPtr<AHiveSportsCar>> CarsToRestore;
	AffectedCars.GetKeys(CarsToRestore);

	for (const TWeakObjectPtr<AHiveSportsCar>& SportsCar : CarsToRestore)
	{
		RestoreCarGrip(SportsCar);
	}

	Super::Destroyed();
}

void ASlickZoneActor::OnSlickOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	AHiveSportsCar* SportsCar = Cast<AHiveSportsCar>(OtherActor);
	if (!SportsCar && OtherComp)
	{
		SportsCar = Cast<AHiveSportsCar>(OtherComp->GetOwner());
	}

	if (SportsCar)
	{
		ApplyToCar(SportsCar);
	}
}

void ASlickZoneActor::ApplyToCar(AHiveSportsCar* SportsCar)
{
	if (!SportsCar)
	{
		return;
	}

	UHiveVehicleMovementComponent* Movement = Cast<UHiveVehicleMovementComponent>(SportsCar->GetChaosVehicleMovement());
	if (!Movement)
	{
		return;
	}

	TWeakObjectPtr<AHiveSportsCar> SportsCarKey(SportsCar);
	FSlickAffectedCarState& AffectedState = AffectedCars.FindOrAdd(SportsCarKey);

	if (AffectedState.AffectedWheelCount <= 0)
	{
		AffectedState.AffectedWheelCount = FMath::Min(4, Movement->Wheels.Num());
	}

	ApplyGripReduction(SportsCarKey);

	GetWorldTimerManager().ClearTimer(AffectedState.RestoreTimerHandle);
	FTimerDelegate RestoreDelegate = FTimerDelegate::CreateUObject(this, &ASlickZoneActor::RestoreCarGrip, SportsCarKey);
	GetWorldTimerManager().SetTimer(AffectedState.RestoreTimerHandle, RestoreDelegate, EffectDuration, false);
}

void ASlickZoneActor::ApplyGripReduction(TWeakObjectPtr<AHiveSportsCar> SportsCar)
{
	AHiveSportsCar* SportsCarPtr = SportsCar.Get();
	FSlickAffectedCarState* AffectedState = AffectedCars.Find(SportsCar);
	if (!SportsCarPtr || !AffectedState)
	{
		return;
	}

	UHiveVehicleMovementComponent* Movement = Cast<UHiveVehicleMovementComponent>(SportsCarPtr->GetChaosVehicleMovement());
	if (!Movement)
	{
		return;
	}

	for (int32 WheelIndex = 0; WheelIndex < AffectedState->AffectedWheelCount; ++WheelIndex)
	{
		Movement->SetExternalFrictionMultiplier(WheelIndex, GripReduction);
	}
}

void ASlickZoneActor::RestoreCarGrip(TWeakObjectPtr<AHiveSportsCar> SportsCar)
{
	AHiveSportsCar* SportsCarPtr = SportsCar.Get();
	FSlickAffectedCarState* AffectedState = AffectedCars.Find(SportsCar);
	if (!AffectedState)
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(AffectedState->RestoreTimerHandle);

	if (SportsCarPtr)
	{
		if (UHiveVehicleMovementComponent* Movement = Cast<UHiveVehicleMovementComponent>(SportsCarPtr->GetChaosVehicleMovement()))
		{
			for (int32 WheelIndex = 0; WheelIndex < AffectedState->AffectedWheelCount; ++WheelIndex)
			{
				Movement->SetExternalFrictionMultiplier(WheelIndex, 1.0f);
			}
		}
	}

	AffectedCars.Remove(SportsCar);
}

void ASlickZoneActor::DestroySlickZone()
{
	Destroy();
}

void ASlickZoneActor::UpdateZoneVisualScale()
{
	if (SlickMesh)
	{
		const float PlaneScale = (SlickRadius * 2.0f) / 100.0f;
		SlickMesh->SetRelativeScale3D(FVector(PlaneScale, PlaneScale, 1.0f));
	}
}

AOilSlick::AOilSlick()
{
	Category = EPowerUpCategory::TrackHazard;
	DisplayName = FText::FromString(TEXT("Oil Slick"));
	Duration = 0.0f;
}

void AOilSlick::OnActivate_Implementation(AHiveSportsCar* InstigatorCar)
{
	AHiveSportsCar* TargetCar = GetActivationTargetCar(InstigatorCar ? InstigatorCar : OwningCar);
	if (!TargetCar || !GetWorld())
	{
		return;
	}

	AHiveSportsCar* SourceCar = GetActivationSourceCar(TargetCar);
	const FVector SourceLocation = GetActivationSourceLocation(TargetCar);
	const FVector SourceForward = GetActivationSourceForwardVector(TargetCar);
	const FVector SpawnLocation = SourceLocation - (SourceForward * 300.0f);
	const FRotator SpawnRotation = SourceForward.Rotation();

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = SourceCar ? SourceCar : TargetCar;
	SpawnParameters.Instigator = SourceCar ? SourceCar : TargetCar;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ASlickZoneActor* SlickZone = GetWorld()->SpawnActor<ASlickZoneActor>(ASlickZoneActor::StaticClass(), SpawnLocation, SpawnRotation, SpawnParameters);
	if (SlickZone)
	{
		SlickZone->Initialize(SlickRadius, GripReduction, SlickDuration, EffectDuration);
	}

	if (DeploySound)
	{
		UGameplayStatics::PlaySoundAtLocation(TargetCar, DeploySound, SpawnLocation);
	}

	SetLifeSpan(1.0f);
}
