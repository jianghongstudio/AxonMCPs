using UnrealBuildTool;

public class AxonLLM : ModuleRules
{
	public AxonLLM(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"AxonCore",
			"AxonKnowledgeLib",
			"UnrealEd",
			"Json",
			"JsonUtilities",
			"HTTP",
			"DeveloperSettings",
			"Projects",
			"PropertyEditor",
			"Slate",
			"SlateCore",
			"InputCore"
		});
	}
}
