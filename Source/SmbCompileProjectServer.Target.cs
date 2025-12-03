// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class SmbCompileProjectServerTarget : TargetRules
{
	public SmbCompileProjectServerTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Server;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		ExtraModuleNames.Add("SmbCompileProject");
	}
}