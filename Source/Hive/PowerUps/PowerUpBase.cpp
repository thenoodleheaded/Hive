// Copyright Epic Games, Inc. All Rights Reserved.

#include "PowerUps/PowerUpBase.h"
#include "Components/SceneComponent.h"
#include "Hive.h"
#include "HiveSportsCar.h"
#include "TimerManager.h"

void APowerUpBase::OnPickup_Implementation(AHiveSportsCar* Carrier)
{
	OwningCar = Carrier;
	HideAllVisualComponents();
}

void APowerUpBase::OnActivate_Implementation(AHiveSportsCar* InstigatorCar)
{
	StartDurationTimer();
}

void APowerUpBase::OnExpire_Implementation()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DurationTimerHandle);
	}

	OwningCar = nullptr;
}

void APowerUpBase::StartDurationTimer()
{
	if (Duration <= 0.0f)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		FTimerDelegate ExpireDelegate;
		ExpireDelegate.BindUFunction(this, GET_FUNCTION_NAME_CHECKED(APowerUpBase, OnExpire));
		World->GetTimerManager().SetTimer(DurationTimerHandle, ExpireDelegate, Duration, false);
	}
}

void APowerUpBase::SetActivationSourceTransform(const FTransform& SourceTransform)
{
	ActivationSourceTransform = SourceTransform;
	ActivationSourceActorOverride = nullptr;
}

void APowerUpBase::SetActivationContext(const FTransform& SourceTransform, AHiveSportsCar* SourceCar, AHiveSportsCar* TargetCar, AActor* SourceActor)
{
	ActivationSourceTransform = SourceTransform;
	bHasActivationSourceCarOverride = true;
	ActivationSourceCarOverride = SourceCar;
	ActivationSourceActorOverride = SourceActor ? SourceActor : SourceCar;
	ActivationTargetCarOverride = TargetCar;

	UE_LOG(LogHive, Verbose, TEXT("[PowerUpBase] ActivationContext PowerUp=%s SourceActor=%s SourceCar=%s TargetCar=%s SourceLocation=%s SourceForward=%s"),
		*GetNameSafe(this),
		*GetNameSafe(ActivationSourceActorOverride.Get()),
		*GetNameSafe(SourceCar),
		*GetNameSafe(TargetCar),
		*SourceTransform.GetLocation().ToCompactString(),
		*SourceTransform.GetRotation().GetForwardVector().ToCompactString());
}

FVector APowerUpBase::GetActivationSourceLocation(AHiveSportsCar* FallbackCar) const
{
	return ActivationSourceTransform.IsSet() ? ActivationSourceTransform->GetLocation() : (FallbackCar ? FallbackCar->GetActorLocation() : GetActorLocation());
}

FVector APowerUpBase::GetActivationSourceForwardVector(AHiveSportsCar* FallbackCar) const
{
	return ActivationSourceTransform.IsSet() ? ActivationSourceTransform->GetRotation().GetForwardVector() : (FallbackCar ? FallbackCar->GetActorForwardVector() : GetActorForwardVector());
}

AHiveSportsCar* APowerUpBase::GetActivationSourceCar(AHiveSportsCar* FallbackCar) const
{
	return bHasActivationSourceCarOverride ? ActivationSourceCarOverride.Get() : FallbackCar;
}

AActor* APowerUpBase::GetActivationSourceActor(AActor* FallbackActor) const
{
	return ActivationSourceActorOverride.IsValid() ? ActivationSourceActorOverride.Get() : FallbackActor;
}

AHiveSportsCar* APowerUpBase::GetActivationTargetCar(AHiveSportsCar* FallbackCar) const
{
	return ActivationTargetCarOverride.IsValid() ? ActivationTargetCarOverride.Get() : FallbackCar;
}

void APowerUpBase::HideAllVisualComponents()
{
	TArray<USceneComponent*> SceneComponents;
	GetComponents<USceneComponent>(SceneComponents);
	for (USceneComponent* SceneComponent : SceneComponents)
	{
		if (SceneComponent)
		{
			SceneComponent->SetVisibility(false, true);
		}
	}
}
