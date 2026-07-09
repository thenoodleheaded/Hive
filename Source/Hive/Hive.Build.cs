// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Hive : ModuleRules
{
	public Hive(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"ChaosVehicles",
			"PhysicsCore",
			"UMG",
			"Slate",
			"Niagara"
		});

		PublicIncludePaths.AddRange(new string[] {
			"Hive",
			"Hive/SportsCar",
			"Hive/OffroadCar",
			"Hive/Variant_OffRoad",
			"Hive/Variant_TimeTrial",
			"Hive/Variant_TimeTrial/UI"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
