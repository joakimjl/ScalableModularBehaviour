// Copyright © 2025 Land Chaunax, All rights reserved.

using UnrealBuildTool;

public class ScalableMassBehaviour : ModuleRules
{
	public ScalableMassBehaviour(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core", "MassRepresentation", "Niagara"
				
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject", "Engine", "Slate",
				"SlateCore", "MassEntity", "MassActors",
				"MassMovement", "MassNavigation", "MassAIBehavior", 
				"MassCrowd", "MassCommon", "Niagara", 
				"MassEQS", "AIModule", "StateTreeModule",
				"MassSignals", "MassSpawner", "AnimationCore",
				"NavigationSystem", "GameplayTags", "MassReplication", 
				"NetCore", "MassLOD"
				
				// ... add private dependencies that you statically link with here ...	
			}
			);
		
		// Editor-only dependencies
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"Blutility",
					"UnrealEd",
					"AnimToTextureEditor",
					"GeometryScriptingCore"
				}
			);
		}

		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
