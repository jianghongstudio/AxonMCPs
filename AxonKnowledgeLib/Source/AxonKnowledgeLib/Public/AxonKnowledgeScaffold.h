#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

struct FAxonKbNamePlan
{
	FString ProjectName;
	FString PluginName;
	FString Namespace;
	FString McpTool;
	FString PluginPath;
};

struct FAxonKbScaffoldRequest
{
	FString ProjectName;   // optional; default = FApp::GetProjectName()
	FString PluginName;    // optional override
	FString Namespace;     // optional override
	bool bDryRun = false;
	bool bForce = false;   // overwrite existing plugin dir
};

struct FAxonKbScaffoldResult
{
	bool bSuccess = false;
	FString ErrorMessage;
	TSharedPtr<FJsonObject> Payload;
};

/** Derive Axon{Project}KB / {snake}_kb and write a thin KB sibling plugin. */
class AXONKNOWLEDGELIB_API FAxonKnowledgeScaffold
{
public:
	static FAxonKbNamePlan DeriveNames(const FString& ProjectName);
	static FAxonKbScaffoldResult Create(const FAxonKbScaffoldRequest& Request);

private:
	static FString ToSnakeCase(const FString& PascalOrMixed);
	static FString SanitizePluginToken(const FString& In);
	static bool IsValidIdentifier(const FString& Name);
	static bool WriteFile(const FString& Path, const FString& Contents, TArray<FString>& Written, FString& OutError);
	static bool MergeUProjectEnablePlugin(const FString& PluginName, TArray<FString>& Written, FString& OutError, bool bDryRun);
	static FString MakeUplugin(const FString& PluginName, const FString& Namespace);
	static FString MakeBuildCs(const FString& ModuleName);
	static FString MakeModuleH(const FString& ModuleName);
	static FString MakeModuleCpp(const FString& ModuleName, const FString& Namespace, const FString& PluginName);
	static FString MakeRoutingMd(const FString& ProjectName, const FString& Namespace);
	static FString MakeReadmeMd(const FString& PluginName, const FString& Namespace, const FString& ProjectName);
};
