// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class UnrealXunFeiSpeech : ModuleRules
{
	public UnrealXunFeiSpeech(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore" });

		PrivateDependencyModuleNames.AddRange(new string[] {  });

        // Uncomment if you are using Slate UI
        // PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

        // Uncomment if you are using online features
        // PrivateDependencyModuleNames.Add("OnlineSubsystem");

        // To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true

        PrivateIncludePaths.Add("UnrealXunFeiSpeech/Private");
        PublicIncludePaths.Add("UnrealXunFeiSpeech/Public");

        PublicLibraryPaths.AddRange(new string[] { "..\\XunFei\\libs" });
        PublicAdditionalLibraries.AddRange(new string[] { "msc_x64.lib" });

        PublicIncludePaths.AddRange(new string[] { "..\\XunFei\\include" });
    }
}
