using UnrealBuildTool;

public class AxonEditor : ModuleRules
{
	public AxonEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine" });
		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"AxonCore", "UnrealEd", "EditorSubsystem", "PropertyEditor", "Slate", "SlateCore",
			"Json", "JsonUtilities", "MessageLog", "RenderCore", "RHI", "ImageWrapper",
			"Niagara", "AssetTools", "EditorScriptingUtilities", "AdvancedPreviewScene", "ImageCore",
			"Projects", "ProceduralMeshComponent", "PythonScriptPlugin", "LevelEditor", "AIModule",
			"EnhancedInput", "UMG", "UMGEditor", "MaterialEditor"
		});
		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			PrivateDependencyModuleNames.Add("LiveCoding");
		}
	}
}