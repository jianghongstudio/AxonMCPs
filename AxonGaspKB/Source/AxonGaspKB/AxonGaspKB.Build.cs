using UnrealBuildTool;

public class AxonGaspKB : ModuleRules
{
	public AxonGaspKB(ReadOnlyTargetRules Target) : base(Target)
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
			"AxonCore",
			"AxonKnowledgeLib"
		});
	}
}
