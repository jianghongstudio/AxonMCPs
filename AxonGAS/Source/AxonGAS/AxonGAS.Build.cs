using UnrealBuildTool;

public class AxonGAS : ModuleRules
{
	private static bool IsPluginEnabled(ReadOnlyTargetRules Target, string PluginName)
	{
		if (Target.ProjectFile == null) return false;
		if (Target.DisablePlugins != null && System.Linq.Enumerable.Contains(Target.DisablePlugins, PluginName)) return false;
		if (Target.EnablePlugins != null && System.Linq.Enumerable.Contains(Target.EnablePlugins, PluginName)) return true;
		try
		{
			ProjectDescriptor Project = ProjectDescriptor.FromFile(Target.ProjectFile);
			if (Project.Plugins != null)
				foreach (PluginReferenceDescriptor Ref in Project.Plugins)
					if (string.Equals(Ref.Name, PluginName, System.StringComparison.OrdinalIgnoreCase) && !Ref.bOptional)
						return Ref.bEnabled && Ref.IsEnabledForPlatform(Target.Platform)
							&& Ref.IsEnabledForTargetConfiguration(Target.Configuration) && Ref.IsEnabledForTarget(Target.Type);
		}
		catch (System.Exception) { return false; }
		return false;
	}

	public AxonGAS(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new[]
		{
			"Core", "CoreUObject", "Engine", "GameplayAbilities", "GameplayTags", "GameplayTasks", "UMG", "Slate", "SlateCore"
		});
		PrivateDependencyModuleNames.AddRange(new[]
		{
			"AxonCore", "UnrealEd", "BlueprintGraph", "GameplayAbilitiesEditor", "GameplayTasksEditor", "GameplayTagsEditor",
			"EnhancedInput", "EditorScriptingUtilities", "Json", "JsonUtilities", "UMGEditor"
		});
		bool bReleaseBuild = System.Environment.GetEnvironmentVariable("AXON_RELEASE_BUILD") == "1";
		bool bHasGBA = !bReleaseBuild && IsPluginEnabled(Target, "BlueprintAttributes");
		if (bHasGBA) { PrivateDependencyModuleNames.Add("BlueprintAttributes"); PublicDefinitions.Add("WITH_GBA=1"); }
		else { PublicDefinitions.Add("WITH_GBA=0"); }
	}
}