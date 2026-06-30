// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HivePawn.h"
#include "HiveSportsCar.generated.h"

/**
 *  Sports car wheeled vehicle implementation
 */
UCLASS(abstract)
class AHiveSportsCar : public AHivePawn
{
	GENERATED_BODY()
	
public:

	AHiveSportsCar();
};
