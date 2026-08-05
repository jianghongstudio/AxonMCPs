using UnrealBuildTool;

public class AxonCore : ModuleRules
{
	public AxonCore(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"HTTP",
			"HTTPServer",
			"Json",
			"JsonUtilities",
			"Slate",
			"SlateCore",
			"DeveloperSettings",
			"Projects",
			"AssetRegistry",
			"EditorSubsystem",
			"UnrealEd",
			"Sockets",
			"Networking",
			// StructUtils / StructUtilsEngine were plugins until UE 5.5; FInstancedStruct
			// now lives in CoreUObject (already a Public dep above).
			"ToolMenus",
			"StatusBar",
			"InputCore",
			"Settings"
		});
	}
}
