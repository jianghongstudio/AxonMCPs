using UnrealBuildTool;

public class AxonSource : ModuleRules
{
	public AxonSource(ReadOnlyTargetRules Target) : base(Target)
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
			"SQLiteCore",
			"EditorSubsystem",
			"UnrealEd",
			"Json",
			"JsonUtilities",
			"Slate",
			"SlateCore",
			"Projects"
		});
	}
}
