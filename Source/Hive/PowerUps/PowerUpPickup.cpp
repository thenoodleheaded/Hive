// Copyright Epic Games, Inc. All Rights Reserved.

#include "PowerUps/PowerUpPickup.h"
#include "Components/PointLightComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/RotatingMovementComponent.h"
#include "Hive.h"
#include "HiveSportsCar.h"
#include "NiagaraComponent.h"
#include "PowerUps/PowerUpBase.h"
#include "PowerUps/PowerUpSlotComponent.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

APowerUpPickup::APowerUpPickup()
{
	PrimaryActorTick.bCanEverTick = false;

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	SetRootComponent(CollisionSphere);
	CollisionSphere->InitSphereRadius(150.0f);
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionSphere->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CollisionSphere->SetCollisionResponseToChannel(ECC_Vehicle, ECR_Overlap);
	CollisionSphere->SetGenerateOverlapEvents(true);
	CollisionSphere->SetCanEverAffectNavigation(false);

	PickupMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PickupMesh"));
	PickupMesh->SetupAttachment(CollisionSphere);
	PickupMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PickupMesh->SetCanEverAffectNavigation(false);
	PickupMesh->SetCastShadow(false);
	PickupMesh->SetRelativeScale3D(FVector(0.5f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMeshFinder(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMeshFinder.Succeeded())
	{
		PickupMesh->SetStaticMesh(SphereMeshFinder.Object);
	}

	PickupLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("PickupLight"));
	PickupLight->SetupAttachment(CollisionSphere);
	PickupLight->SetRelativeLocation(FVector(0.0f, 0.0f, 120.0f));
	PickupLight->SetLightColor(PickupLightColor);
	PickupLight->SetIntensity(2000.0f);
	PickupLight->SetAttenuationRadius(300.0f);
	PickupLight->SetCastShadows(false);

	RotatingMovement = CreateDefaultSubobject<URotatingMovementComponent>(TEXT("RotatingMovement"));
	RotatingMovement->RotationRate = FRotator(0.0f, 90.0f, 0.0f);
}

void APowerUpPickup::BeginPlay()
{
	Super::BeginPlay();

	CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &APowerUpPickup::OnPickupOverlap);

	if (!PowerUpClass)
	{
		UE_LOG(LogHive, Warning, TEXT("%s has no PowerUpClass assigned. Pickup collision disabled."), *GetName());
		CollisionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void APowerUpPickup::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (PickupLight)
	{
		PickupLight->SetLightColor(PickupLightColor);
	}
}

void APowerUpPickup::Respawn()
{
	SetPickupActive(true);

	if (RespawnEffect)
	{
		RespawnEffect->SetWorldLocation(GetActorLocation());
		RespawnEffect->SetVisibility(true, true);
		RespawnEffect->Activate(true);
	}
}

void APowerUpPickup::OnPickupOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	AHiveSportsCar* SportsCar = Cast<AHiveSportsCar>(OtherActor);
	if (!SportsCar && OtherComp)
	{
		SportsCar = Cast<AHiveSportsCar>(OtherComp->GetOwner());
	}

	if (!SportsCar || !PowerUpClass)
	{
		return;
	}

	UPowerUpSlotComponent* SlotComponent = SportsCar->FindComponentByClass<UPowerUpSlotComponent>();
	if (!SlotComponent)
	{
		UE_LOG(LogHive, Warning, TEXT("%s overlapped %s, but the car has no PowerUpSlotComponent."), *SportsCar->GetName(), *GetName());
		return;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = SportsCar;
	SpawnParameters.Instigator = SportsCar;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	APowerUpBase* PowerUp = GetWorld() ? GetWorld()->SpawnActor<APowerUpBase>(PowerUpClass, GetActorTransform(), SpawnParameters) : nullptr;
	if (!PowerUp)
	{
		return;
	}

	if (SlotComponent->TryAddPowerUp(PowerUp))
	{
		PowerUp->SetActorEnableCollision(false);
		SetPickupActive(false);

		if (bAutoRespawn)
		{
			GetWorldTimerManager().SetTimer(RespawnTimerHandle, this, &APowerUpPickup::Respawn, RespawnTime, false);
		}
	}
	else
	{
		PowerUp->Destroy();
	}
}

void APowerUpPickup::SetPickupActive(bool bIsActive)
{
	if (PickupMesh)
	{
		PickupMesh->SetVisibility(bIsActive, true);
	}

	if (PickupLight)
	{
		PickupLight->SetVisibility(bIsActive);
	}

	if (CollisionSphere)
	{
		CollisionSphere->SetCollisionEnabled(bIsActive && PowerUpClass ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
	}
}
