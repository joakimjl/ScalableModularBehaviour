// Copyright © 2025 Land Chaunax, All rights reserved.

#include "ScalableMassBehaviour.h"
#include "MassSettings.h"
#include "MassBehaviorSettings.h"
#include "SmbSignalProcessor.h"

#define LOCTEXT_NAMESPACE "FScalableMassBehaviourModule"

void FScalableMassBehaviourModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
#if WITH_EDITOR
	UMassSettings* Settings  = GetMutableDefault<UMassSettings>();
	UMassBehaviorSettings* BehaviorSettings = static_cast<UMassBehaviorSettings*>(Settings->ModuleSettings["Mass Behavior"]);
	if (BehaviorSettings && BehaviorSettings->DynamicStateTreeProcessorClass.GetAssetName() == UMassStateTreeProcessor::StaticClass()->GetFName())
	{
		BehaviorSettings->DynamicStateTreeProcessorClass = USmbSignalProcessor::StaticClass();
		
		BehaviorSettings->MarkPackageDirty();
		BehaviorSettings->SaveConfig(CPF_Config, *BehaviorSettings->GetDefaultConfigFilename());
		
		Settings->MarkPackageDirty();
		Settings->SaveConfig(CPF_Config, *Settings->GetDefaultConfigFilename());
		
		BehaviorSettings->TryUpdateDefaultConfigFile();
		Settings->TryUpdateDefaultConfigFile();
	}
#endif
	
}

void FScalableMassBehaviourModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

UE_DEFINE_GAMEPLAY_TAG_COMMENT(Skill, "Skill", "Scalable Modular Behaviour Skills");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Skill_Type, "Skill.Type", "Types of Skills, uses Animations from related State");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(Skill_Modifier,			"Skill.Modifier",			"Base tag for skill modifiers");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(Skill_Type_Melee,		"Skill.Type.Melee",			"Uses Melee Attack Animation");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Skill_Type_Ranged,		"Skill.Type.Ranged",		"(Experimental) Uses Ranged Attack Animations");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Skill_Type_Cast,			"Skill.Type.Cast",			"(TBD) Uses Cast Animations");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Skill_Type_Gather,		"Skill.Type.Gather",		"(TBD) Uses Gathering Animations AND collects resource");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(Skill_Modifier_Area,			"Skill.Modifier.Area",		"Area modifier");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Skill_Modifier_Single,		"Skill.Modifier.Single",	"Single target modifier");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Skill_Modifier_Attack,		"Skill.Modifier.Attack",	"Attack modifier");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(Skill_Modifier_Fire,			"Skill.Modifier.Fire",		"(TBD) Fire modifier");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Skill_Modifier_Freeze,		"Skill.Modifier.Freeze",	"(TBD) Freeze modifier");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Skill_Modifier_Electric,		"Skill.Modifier.Electric",	"(TBD) Electric modifier");

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FScalableMassBehaviourModule, ScalableMassBehaviour)