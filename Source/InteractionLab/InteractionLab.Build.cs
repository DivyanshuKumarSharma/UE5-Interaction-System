// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class InteractionLab : ModuleRules
{
	public InteractionLab(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] {
			"InteractionSystem"
		});

		PublicIncludePaths.AddRange(new string[] {
			"InteractionLab",
			"InteractionLab/Interactables",
			"InteractionLab/Variant_Platforming",
			"InteractionLab/Variant_Platforming/Animation",
			"InteractionLab/Variant_Combat",
			"InteractionLab/Variant_Combat/AI",
			"InteractionLab/Variant_Combat/Animation",
			"InteractionLab/Variant_Combat/Gameplay",
			"InteractionLab/Variant_Combat/Interfaces",
			"InteractionLab/Variant_Combat/UI",
			"InteractionLab/Variant_SideScrolling",
			"InteractionLab/Variant_SideScrolling/AI",
			"InteractionLab/Variant_SideScrolling/Gameplay",
			"InteractionLab/Variant_SideScrolling/Interfaces",
			"InteractionLab/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
