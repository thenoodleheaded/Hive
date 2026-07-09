// Copyright Epic Games, Inc. All Rights Reserved.

#include "PowerUps/SlowField.h"
#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Hive.h"
#include "HiveSportsCar.h"
#include "HiveVehicleMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr float DefaultSlowFieldSpeedMultiplier = 0.7f;

	UHiveVehicleMovementComponent* GetHiveMovement(AHiveSportsCar* SportsCar)
	{
		return SportsCar ? Cast<UHiveVehicleMovementComponent>(SportsCar->GetChaosVehicleMovement()) : nullptr;
	}

	float SanitizeSpeedMultiplier(float InSpeedMultiplier, const UObject* Context)
	{
		if (InSpeedMultiplier <= KINDA_SMALL_NUMBER)
		{
			UE_LOG(LogHive, Warning, TEXT("[SlowField] %s had invalid SpeedMultiplier %.3f. Falling back to %.3f."),
				*GetNameSafe(Context),
				InSpeedMultiplier,
				DefaultSlowFieldSpeedMultiplier);
			return DefaultSlowFieldSpeedMultiplier;
		}

		return FMath::Clamp(InSpeedMultiplier, 0.1f, 0.9f);
	}
}

ASlowFieldActor::ASlowFieldActor()
{
	PrimaryActorTick.bCanEverTick = false;

	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	SetRootComponent(CollisionBox);
	CollisionBox->InitBoxExtent(FieldExtent);
	CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionBox->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CollisionBox->SetCollisionResponseToChannel(ECC_Vehicle, ECR_Overlap);
	CollisionBox->SetGenerateOverlapEvents(true);
	CollisionBox->SetCanEverAffectNavigation(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	for (int32 EdgeIndex = 0; EdgeIndex < 12; ++EdgeIndex)
	{
		UStaticMeshComponent* EdgeMesh = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("FieldEdge_%02d"), EdgeIndex));
		EdgeMesh->SetupAttachment(CollisionBox);
		EdgeMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		EdgeMesh->SetCanEverAffectNavigation(false);
		EdgeMesh->SetCastShadow(false);
		if (CubeMeshFinder.Succeeded())
		{
			EdgeMesh->SetStaticMesh(CubeMeshFinder.Object);
		}
		EdgeMeshes.Add(EdgeMesh);
	}

	FieldEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("FieldEffect"));
	FieldEffect->SetupAttachment(CollisionBox);
	FieldEffect->bAutoActivate = false;
}

void ASlowFieldActor::Initialize(AHiveSportsCar* InSourceCar, const FVector& InFieldExtent, float InFieldDuration, float InSpeedMultiplier, bool bInAffectSelf, USoundBase* InLoopSound)
{
	SourceCar = InSourceCar;
	FieldExtent = InFieldExtent;
	FieldDuration = InFieldDuration;
	SpeedMultiplier = SanitizeSpeedMultiplier(InSpeedMultiplier, this);
	bAffectSelf = bInAffectSelf;
	LoopSound = InLoopSound;
	bHasInitialized = true;

	if (CollisionBox)
	{
		CollisionBox->SetBoxExtent(FieldExtent);
		CollisionBox->OnComponentBeginOverlap.AddDynamic(this, &ASlowFieldActor::OnSlowFieldBeginOverlap);
		CollisionBox->OnComponentEndOverlap.AddDynamic(this, &ASlowFieldActor::OnSlowFieldEndOverlap);
	}

	UpdateFieldVisualScale();

	UE_LOG(LogHive, Verbose, TEXT("[SlowField] Spawn initialized. SpeedMultiplier=%.3f FieldExtent=%s SourceCar=%s bAffectSelf=%s"),
		SpeedMultiplier,
		*FieldExtent.ToCompactString(),
		*GetNameSafe(InSourceCar),
		bAffectSelf ? TEXT("true") : TEXT("false"));

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FieldLifetimeTimerHandle);
		World->GetTimerManager().SetTimer(FieldLifetimeTimerHandle, this, &ASlowFieldActor::DestroySlowField, FieldDuration, false);
	}

	RefreshAffectedCars();
}

void ASlowFieldActor::BeginPlay()
{
	Super::BeginPlay();

	if (CollisionBox)
	{
		CollisionBox->SetBoxExtent(FieldExtent);
	}

	UpdateFieldVisualScale();

	if (FieldEffect)
	{
		FieldEffect->SetVisibility(true, true);
		FieldEffect->Activate(true);
	}

	if (LoopSound)
	{
		LoopAudioComponent = UGameplayStatics::SpawnSoundAtLocation(this, LoopSound, GetActorLocation());
	}

	if (bHasInitialized)
	{
		RefreshAffectedCars();
	}

	if (!GetWorldTimerManager().IsTimerActive(FieldLifetimeTimerHandle))
	{
		GetWorldTimerManager().SetTimer(FieldLifetimeTimerHandle, this, &ASlowFieldActor::DestroySlowField, FieldDuration, false);
	}
}

void ASlowFieldActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (CollisionBox)
	{
		CollisionBox->SetBoxExtent(FieldExtent);
	}

	UpdateFieldVisualScale();
}

void ASlowFieldActor::Destroyed()
{
	TArray<TWeakObjectPtr<AHiveSportsCar>> CarsToRestore;
	AffectedCars.GetKeys(CarsToRestore);
	for (const TWeakObjectPtr<AHiveSportsCar>& SportsCar : CarsToRestore)
	{
		RestoreCarSpeed(SportsCar.Get());
	}
	AffectedCars.Reset();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FieldLifetimeTimerHandle);
	}

	if (LoopAudioComponent)
	{
		LoopAudioComponent->Stop();
	}

	if (FieldEffect)
	{
		FieldEffect->Deactivate();
	}

	Super::Destroyed();
}

void ASlowFieldActor::OnSlowFieldBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	AHiveSportsCar* SportsCar = Cast<AHiveSportsCar>(OtherActor);
	if (!SportsCar && OtherComp)
	{
		SportsCar = Cast<AHiveSportsCar>(OtherComp->GetOwner());
	}

	if (SportsCar)
	{
		ApplySlowToCar(SportsCar);
	}
}

void ASlowFieldActor::OnSlowFieldEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	AHiveSportsCar* SportsCar = Cast<AHiveSportsCar>(OtherActor);
	if (!SportsCar && OtherComp)
	{
		SportsCar = Cast<AHiveSportsCar>(OtherComp->GetOwner());
	}

	if (!SportsCar)
	{
		return;
	}

	if (CollisionBox && CollisionBox->IsOverlappingActor(SportsCar))
	{
		return;
	}

	RestoreCarSpeed(SportsCar);
}

void ASlowFieldActor::RefreshAffectedCars()
{
	if (!CollisionBox)
	{
		return;
	}

	if (!bHasInitialized)
	{
		return;
	}

	TArray<AActor*> OverlappingActors;
	CollisionBox->GetOverlappingActors(OverlappingActors, AHiveSportsCar::StaticClass());
	TSet<TWeakObjectPtr<AHiveSportsCar>> CurrentOverlappingCars;

	for (AActor* Actor : OverlappingActors)
	{
		AHiveSportsCar* SportsCar = Cast<AHiveSportsCar>(Actor);
		if (!SportsCar)
		{
			continue;
		}

		if (SportsCar == SourceCar.Get() && !bAffectSelf)
		{
			continue;
		}

		CurrentOverlappingCars.Add(SportsCar);
		ApplySlowToCar(SportsCar);
	}

	TArray<TWeakObjectPtr<AHiveSportsCar>> AffectedCarKeys;
	AffectedCars.GetKeys(AffectedCarKeys);
	for (const TWeakObjectPtr<AHiveSportsCar>& AffectedCar : AffectedCarKeys)
	{
		if (!CurrentOverlappingCars.Contains(AffectedCar))
		{
			RestoreCarSpeed(AffectedCar.Get());
		}
	}

}

void ASlowFieldActor::ApplySlowToCar(AHiveSportsCar* SportsCar)
{
	if (!SportsCar || (SportsCar == SourceCar.Get() && !bAffectSelf))
	{
		return;
	}

	UHiveVehicleMovementComponent* HiveMovement = GetHiveMovement(SportsCar);
	if (!HiveMovement)
	{
		UE_LOG(LogHive, Warning, TEXT("[SlowField] %s has no UHiveVehicleMovementComponent; cannot apply speed cap."),
			*GetNameSafe(SportsCar));
		return;
	}

	FSlowFieldAffectedCarState* ExistingState = AffectedCars.Find(SportsCar);
	if (!ExistingState)
	{
		const float EntrySpeed = SportsCar->GetVelocity().Size();
		const float SpeedCap = EntrySpeed * SanitizeSpeedMultiplier(SpeedMultiplier, this);
		FSlowFieldAffectedCarState& NewState = AffectedCars.Add(SportsCar);
		NewState.EntrySpeed = EntrySpeed;
		NewState.SpeedCap = SpeedCap;

		UE_LOG(LogHive, Verbose, TEXT("[SlowField] ENTER %s EntrySpeed=%.1f SpeedMultiplier=%.3f SpeedCap=%.1f"),
			*GetNameSafe(SportsCar),
			EntrySpeed,
			SpeedMultiplier,
			SpeedCap);

		HiveMovement->ApplySpeedCap(SpeedCap);
		return;
	}

	HiveMovement->ApplySpeedCap(ExistingState->SpeedCap);
}

void ASlowFieldActor::RestoreCarSpeed(AHiveSportsCar* SportsCar)
{
	if (!SportsCar)
	{
		return;
	}

	const FSlowFieldAffectedCarState* AffectedState = AffectedCars.Find(SportsCar);
	UE_LOG(LogHive, Verbose, TEXT("[SlowField] EXIT %s LastSpeed=%.1f EntrySpeed=%.1f SpeedCap=%.1f"),
		*GetNameSafe(SportsCar),
		SportsCar->GetVelocity().Size(),
		AffectedState ? AffectedState->EntrySpeed : 0.0f,
		AffectedState ? AffectedState->SpeedCap : 0.0f);

	if (UHiveVehicleMovementComponent* HiveMovement = GetHiveMovement(SportsCar))
	{
		HiveMovement->ClearSpeedCap();
	}

	AffectedCars.Remove(SportsCar);
}

void ASlowFieldActor::DestroySlowField()
{
	Destroy();
}

void ASlowFieldActor::UpdateFieldVisualScale()
{
	if (EdgeMeshes.Num() < 12)
	{
		return;
	}

	const FVector Extent = FieldExtent.GetAbs();
	const float Thickness = FMath::Max(1.0f, EdgeThickness);

	auto ConfigureEdge = [this](int32 EdgeIndex, const FVector& RelativeLocation, const FVector& Dimensions)
	{
		if (!EdgeMeshes.IsValidIndex(EdgeIndex) || !EdgeMeshes[EdgeIndex])
		{
			return;
		}

		EdgeMeshes[EdgeIndex]->SetRelativeLocation(RelativeLocation);
		EdgeMeshes[EdgeIndex]->SetRelativeScale3D(Dimensions / 100.0f);
	};

	ConfigureEdge(0, FVector(0.0f, Extent.Y, Extent.Z), FVector(Extent.X * 2.0f, Thickness, Thickness));
	ConfigureEdge(1, FVector(0.0f, -Extent.Y, Extent.Z), FVector(Extent.X * 2.0f, Thickness, Thickness));
	ConfigureEdge(2, FVector(0.0f, Extent.Y, -Extent.Z), FVector(Extent.X * 2.0f, Thickness, Thickness));
	ConfigureEdge(3, FVector(0.0f, -Extent.Y, -Extent.Z), FVector(Extent.X * 2.0f, Thickness, Thickness));

	ConfigureEdge(4, FVector(Extent.X, 0.0f, Extent.Z), FVector(Thickness, Extent.Y * 2.0f, Thickness));
	ConfigureEdge(5, FVector(-Extent.X, 0.0f, Extent.Z), FVector(Thickness, Extent.Y * 2.0f, Thickness));
	ConfigureEdge(6, FVector(Extent.X, 0.0f, -Extent.Z), FVector(Thickness, Extent.Y * 2.0f, Thickness));
	ConfigureEdge(7, FVector(-Extent.X, 0.0f, -Extent.Z), FVector(Thickness, Extent.Y * 2.0f, Thickness));

	ConfigureEdge(8, FVector(Extent.X, Extent.Y, 0.0f), FVector(Thickness, Thickness, Extent.Z * 2.0f));
	ConfigureEdge(9, FVector(-Extent.X, Extent.Y, 0.0f), FVector(Thickness, Thickness, Extent.Z * 2.0f));
	ConfigureEdge(10, FVector(Extent.X, -Extent.Y, 0.0f), FVector(Thickness, Thickness, Extent.Z * 2.0f));
	ConfigureEdge(11, FVector(-Extent.X, -Extent.Y, 0.0f), FVector(Thickness, Thickness, Extent.Z * 2.0f));
}

ASlowField::ASlowField()
{
	Category = EPowerUpCategory::AreaZone;
	DisplayName = FText::FromString(TEXT("Slow Field"));
	Duration = 0.0f;
}

void ASlowField::OnActivate_Implementation(AHiveSportsCar* InstigatorCar)
{
	AHiveSportsCar* TargetCar = GetActivationTargetCar(InstigatorCar ? InstigatorCar : OwningCar);
	if (!TargetCar || !GetWorld())
	{
		return;
	}

	AHiveSportsCar* SourceCar = GetActivationSourceCar(TargetCar);
	const FVector SpawnLocation = GetActivationSourceLocation(TargetCar);
	const FRotator SpawnRotation = GetActivationSourceForwardVector(TargetCar).Rotation();

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = SourceCar ? SourceCar : TargetCar;
	SpawnParameters.Instigator = SourceCar ? SourceCar : TargetCar;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ASlowFieldActor* SlowFieldActor = GetWorld()->SpawnActor<ASlowFieldActor>(ASlowFieldActor::StaticClass(), SpawnLocation, SpawnRotation, SpawnParameters);
	if (SlowFieldActor)
	{
		const float EffectiveSpeedMultiplier = SanitizeSpeedMultiplier(SpeedMultiplier, this);
		UE_LOG(LogHive, Verbose, TEXT("[SlowField] Activating from %s. PowerUpSpeedMultiplier=%.3f EffectiveSpeedMultiplier=%.3f FieldExtent=%s"),
			*GetNameSafe(TargetCar),
			SpeedMultiplier,
			EffectiveSpeedMultiplier,
			*FieldExtent.ToCompactString());

		SlowFieldActor->Initialize(SourceCar, FieldExtent, FieldDuration, EffectiveSpeedMultiplier, bAffectSelf, LoopSound);

		if (FieldEffect && SlowFieldActor->FieldEffect)
		{
			SlowFieldActor->FieldEffect->SetAsset(FieldEffect->GetAsset());
			SlowFieldActor->FieldEffect->SetVisibility(true, true);
			SlowFieldActor->FieldEffect->Activate(true);
		}
	}

	if (DeploySound)
	{
		UGameplayStatics::PlaySoundAtLocation(TargetCar, DeploySound, SpawnLocation);
	}

	SetLifeSpan(1.0f);
}
