using UnrealBuildTool;

public class AxonEditor : ModuleRules
{
	public AxonEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"AxonCore",
			"UnrealEd",
			"LevelEditor",
			"Engine",
			"Json",
			"JsonUtilities",
			"EditorSubsystem",
			"Slate",
			"SlateCore",
			"PythonScriptPlugin"
		});
	}
}
