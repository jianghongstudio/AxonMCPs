using UnrealBuildTool;

public class AxonKnowledgeLib : ModuleRules
{
	public AxonKnowledgeLib(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"UnrealEd",
			"Json",
			"Projects",
			"AxonCore"
		});
	}
}
