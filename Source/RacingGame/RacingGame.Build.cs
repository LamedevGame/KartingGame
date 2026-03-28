// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class RacingGame : ModuleRules
{
	public RacingGame(ReadOnlyTargetRules Target) : base(Target)
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
			"Slate"
		});

		PublicIncludePaths.AddRange(new string[] {
			"RacingGame",
			"RacingGame/Interfaces",
			"RacingGame/GameSetup",
			"RacingGame/Vehicles",
			"RacingGame/Tools",
			"RacingGame/Race"
		});

		PrivateDependencyModuleNames.AddRange(new string[] {
			"OnlineSubsystem",
			"OnlineSubsystemSteam"
		});
	}
}
