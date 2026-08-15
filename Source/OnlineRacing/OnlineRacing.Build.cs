// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class OnlineRacing : ModuleRules
{
	public OnlineRacing(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"AIModule",
			"ChaosVehicles",
			"Core",
			"CoreUObject",
			"Engine",
			"EnhancedInput",
			"InputCore",
			"PhysicsCore",
			"UMG"
		});

		PublicIncludePaths.AddRange(new string[]
		{
			"OnlineRacing",
			"OnlineRacing/OffroadCar",
			"OnlineRacing/SportsCar",
			"OnlineRacing/Variant_OffRoad",
			"OnlineRacing/Variant_TimeTrial",
			"OnlineRacing/Variant_TimeTrial/UI"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"AudioMixer",
			"ChaosVehiclesCore",
			"MotoSynth",
			"SignalProcessing",
			"Slate"
		});
	}
}
