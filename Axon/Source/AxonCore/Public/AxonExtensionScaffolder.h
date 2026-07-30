#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

struct FAxonScaffoldActionSpec
{
	FString Name;
	FString Description;
	TSharedPtr<FJsonObject> ParamsSchema; // optional JSON object of param defs
};

struct FAxonScaffoldRequest
{
	FString PluginName;
	FString Namespace;
	FString Intent;
	TArray<FString> DependPlugins;
	TArray<FString> BuildModules;
	TArray<FAxonScaffoldActionSpec> Actions;
	bool bSkeletonOnly = false;
	FString ResearchRef;
	bool bDryRun = false;
};

struct FAxonScaffoldResult
{
	bool bSuccess = false;
	FString ErrorMessage;
	TSharedPtr<FJsonObject> Payload;
};

/**
 * Writes a generic Axon sibling plugin from Templates/ExtensionPlugin.
 * No business-domain special cases.
 */
class FAxonExtensionScaffolder
{
public:
	static FAxonScaffoldResult Create(const FAxonScaffoldRequest& Request);

	/** List external recipe JSON files under Templates/ExtensionRecipes (may be empty). */
	static TSharedPtr<FJsonObject> ListRecipes(FString& OutError);

	/** Load one external recipe by id (filename without .json). */
	static bool LoadRecipe(const FString& RecipeId, FAxonScaffoldRequest& InOutRequest, FString& OutError);

private:
	static FString GetAxonPluginBaseDir();
	static FString GetTemplatesDir();
	static FString GetRecipesDir();
	static bool IsValidIdentifier(const FString& Name, bool bAllowSnakeCase);
	static FString SanitizeNamespace(const FString& In);
	static FString LoadTemplate(const FString& RelativeName, FString& OutError);
	static FString ApplyPlaceholders(FString Text, const TMap<FString, FString>& Vars);
	static bool WriteFile(const FString& Path, const FString& Contents, TArray<FString>& Written, FString& OutError);
	static bool MergeUProjectEnablePlugin(const FString& PluginName, TArray<FString>& Written, FString& OutError, bool bDryRun);
	static FString MakeDependPluginsJson(const TArray<FString>& DependPlugins);
	static FString MakeExtraBuildModules(const TArray<FString>& BuildModules);
	static FString MakeActionDeclarations(const TArray<FAxonScaffoldActionSpec>& Actions, const FString& ModuleName);
	static FString MakeActionsCpp(const FAxonScaffoldRequest& Request, const FString& ModuleName);
	static FString MakeModuleCpp(const FAxonScaffoldRequest& Request, const FString& ModuleName);
	static FString ToHandlerName(const FString& ActionSnake);
};
