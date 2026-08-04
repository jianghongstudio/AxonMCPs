using UnrealBuildTool;
using System.IO;

public class AxonRewindDebugger : ModuleRules
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

	public AxonRewindDebugger(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new[]
		{
			"Core", "CoreUObject", "Engine"
		});

		PrivateDependencyModuleNames.AddRange(new[]
		{
			"AxonCore",
			"UnrealEd",
			"Json",
			"JsonUtilities",
			"RewindDebuggerInterface",
			// StopRecording/IsRecording are exported on FRewindDebuggerRuntime
			// (REWINDDEBUGGERRUNTIME_API) in this engine; TraceBasedDebuggers does not exist.
			"RewindDebuggerRuntime",
			"GameplayInsights",
			"TraceServices",
			"TraceAnalysis"
		});

		bool bReleaseBuild = System.Environment.GetEnvironmentVariable("AXON_RELEASE_BUILD") == "1";
		// CameraBlueprint may be EnabledByDefault / pulled via game modules without an explicit
		// Non-optional Plugins[] entry — also accept sibling plugin on disk.
		string CameraBpUplugin = Path.GetFullPath(Path.Combine(ModuleDirectory, "..", "..", "..", "..", "CameraBlueprint", "CameraBlueprint.uplugin"));
		bool bHasCameraBP = !bReleaseBuild && (
			IsPluginEnabled(Target, "CameraBlueprint")
			|| IsPluginEnabled(Target, "AxonCameraBlueprint")
			|| File.Exists(CameraBpUplugin));
		if (bHasCameraBP)
		{
			PrivateDependencyModuleNames.AddRange(new[] { "CameraBlueprint", "CameraEditor" });
			PrivateIncludePathModuleNames.Add("CameraEditor");
			// ICameraTraceProvider lives under CameraEditor Private/Trace/
			PrivateIncludePaths.Add(Path.GetFullPath(Path.Combine(ModuleDirectory, "..", "..", "..", "..", "CameraBlueprint", "Source", "CameraEditor", "Private")));
			PublicDefinitions.Add("WITH_CAMERA_BLUEPRINT=1");
		}
		else
		{
			PublicDefinitions.Add("WITH_CAMERA_BLUEPRINT=0");
		}
	}
}