// Copyright Epic Games, Inc. All Rights Reserved.

#include "HiveGameMode.h"
#include "HivePlayerController.h"

AHiveGameMode::AHiveGameMode()
{
	PlayerControllerClass = AHivePlayerController::StaticClass();
}
