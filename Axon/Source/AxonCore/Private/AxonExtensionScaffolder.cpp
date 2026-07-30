#include "AxonExtensionScaffolder.h"
#include "AxonJsonUtils.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "HAL/FileManager.h"

namespace AxonScaffoldInternal
{
	static constexpr int32 ErrNotImplemented = -32020;
}

FString FAxonExtensionScaffolder::GetAxonPluginBaseDir()
{
	TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("Axon"));
	if (Plugin.IsValid())
	{
		return Plugin->GetBaseDir();
	}
	return FPaths::Combine(FPaths::ProjectPluginsDir(), TEXT("Axon"));
}

FString FAxonExtensionScaffolder::GetTemplatesDir()
{
	return GetAxonPluginBaseDir() / TEXT("Templates") / TEXT("ExtensionPlugin");
}

FString FAxonExtensionScaffolder::GetRecipesDir()
{
	return GetAxonPluginBaseDir() / TEXT("Templates") / TEXT("ExtensionRecipes");
}

bool FAxonExtensionScaffolder::IsValidIdentifier(const FString& Name, bool bAllowSnakeCase)
{
	if (Name.IsEmpty())
	{
		return false;
	}
	if (!(FChar::IsAlpha(Name[0]) || Name[0] == TEXT('_')))
	{
		return false;
	}
	for (TCHAR Ch : Name)
	{
		if (FChar::IsAlnum(Ch) || Ch == TEXT('_'))
		{
			continue;
		}
		return false;
	}
	if (!bAllowSnakeCase)
	{
		// Plugin/module names: no requirement beyond alnum/_
	}
	return true;
}

FString FAxonExtensionScaffolder::SanitizeNamespace(const FString& In)
{
	FString Out;
	Out.Reserve(In.Len());
	for (TCHAR Ch : In)
	{
		if (FChar::IsAlnum(Ch))
		{
			Out.AppendChar(FChar::ToLower(Ch));
		}
		else if (Ch == TEXT('-') || Ch == TEXT(' ') || Ch == TEXT('_'))
		{
			Out.AppendChar(TEXT('_'));
		}
	}
	while (Out.Contains(TEXT("__")))
	{
		Out.ReplaceInline(TEXT("__"), TEXT("_"));
	}
	while (Out.StartsWith(TEXT("_")))
	{
		Out.RightChopInline(1);
	}
	while (Out.EndsWith(TEXT("_")))
	{
		Out.LeftChopInline(1);
	}
	return Out;
}

FString FAxonExtensionScaffolder::LoadTemplate(const FString& RelativeName, FString& OutError)
{
	const FString Path = GetTemplatesDir() / RelativeName;
	FString Text;
	if (!FFileHelper::LoadFileToString(Text, *Path))
	{
		OutError = FString::Printf(TEXT("Missing scaffold template: %s"), *Path);
		return FString();
	}
	return Text;
}

FString FAxonExtensionScaffolder::ApplyPlaceholders(FString Text, const TMap<FString, FString>& Vars)
{
	for (const TPair<FString, FString>& Pair : Vars)
	{
		Text.ReplaceInline(*FString::Printf(TEXT("{{%s}}"), *Pair.Key), *Pair.Value);
	}
	return Text;
}

bool FAxonExtensionScaffolder::WriteFile(const FString& Path, const FString& Contents, TArray<FString>& Written, FString& OutError)
{
	const FString Dir = FPaths::GetPath(Path);
	IFileManager::Get().MakeDirectory(*Dir, true);
	if (!FFileHelper::SaveStringToFile(Contents, *Path, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{
		OutError = FString::Printf(TEXT("Failed to write file: %s"), *Path);
		return false;
	}
	Written.Add(Path);
	return true;
}

FString FAxonExtensionScaffolder::MakeDependPluginsJson(const TArray<FString>& DependPlugins)
{
	TArray<FString> Entries;
	Entries.Add(TEXT("\t\t{\n\t\t\t\"Name\": \"Axon\",\n\t\t\t\"Enabled\": true\n\t\t}"));
	for (const FString& Dep : DependPlugins)
	{
		if (Dep.Equals(TEXT("Axon"), ESearchCase::IgnoreCase))
		{
			continue;
		}
		Entries.Add(FString::Printf(
			TEXT("\t\t{\n\t\t\t\"Name\": \"%s\",\n\t\t\t\"Enabled\": true\n\t\t}"), *Dep));
	}
	return FString::Join(Entries, TEXT(",\n"));
}

FString FAxonExtensionScaffolder::MakeExtraBuildModules(const TArray<FString>& BuildModules)
{
	if (BuildModules.Num() == 0)
	{
		return FString();
	}
	FString Extra;
	for (const FString& Mod : BuildModules)
	{
		Extra += FString::Printf(TEXT(",\n\t\t\t\"%s\""), *Mod);
	}
	return Extra;
}

FString FAxonExtensionScaffolder::ToHandlerName(const FString& ActionSnake)
{
	TArray<FString> Parts;
	ActionSnake.ParseIntoArray(Parts, TEXT("_"), true);
	FString Out = TEXT("Handle");
	for (FString& P : Parts)
	{
		if (P.Len() > 0)
		{
			P[0] = FChar::ToUpper(P[0]);
			Out += P;
		}
	}
	return Out;
}

FString FAxonExtensionScaffolder::MakeActionDeclarations(const TArray<FAxonScaffoldActionSpec>& Actions, const FString& ModuleName)
{
	FString Out;
	for (const FAxonScaffoldActionSpec& A : Actions)
	{
		Out += FString::Printf(
			TEXT("\n\tstatic FAxonActionResult %s(const TSharedPtr<FJsonObject>& Params);"),
			*ToHandlerName(A.Name));
	}
	return Out;
}

FString FAxonExtensionScaffolder::MakeActionsCpp(const FAxonScaffoldRequest& Request, const FString& ModuleName)
{
	FString Out;
	Out += FString::Printf(TEXT("#include \"%sActions.h\"\n"), *ModuleName);
	Out += TEXT("#include \"AxonParamSchema.h\"\n");
	Out += TEXT("#include \"AxonJsonUtils.h\"\n\n");
	Out += FString::Printf(
		TEXT("// Scaffolded by axon_create_extension. Intent: %s\n"),
		Request.Intent.IsEmpty() ? TEXT("(none)") : *Request.Intent);
	if (!Request.ResearchRef.IsEmpty())
	{
		Out += FString::Printf(TEXT("// research_ref: %s\n"), *Request.ResearchRef);
	}
	Out += TEXT("// Stub handlers return -32020 until you implement real logic.\n\n");

	Out += FString::Printf(TEXT("void F%sActions::RegisterAll()\n{\n"), *ModuleName);
	Out += TEXT("\tFAxonToolRegistry& Registry = FAxonToolRegistry::Get();\n\n");

	for (const FAxonScaffoldActionSpec& A : Request.Actions)
	{
		const FString Handler = ToHandlerName(A.Name);
		Out += TEXT("\t{\n");
		Out += TEXT("\t\tTSharedPtr<FJsonObject> Schema = MakeShared<FJsonObject>();\n");
		FString EscapedDesc = A.Description;
		EscapedDesc.ReplaceInline(TEXT("\\"), TEXT("\\\\"));
		EscapedDesc.ReplaceInline(TEXT("\""), TEXT("\\\""));
		EscapedDesc.ReplaceInline(TEXT("\n"), TEXT(" "));
		Out += FString::Printf(
			TEXT("\t\tRegistry.RegisterAction(\n\t\t\tTEXT(\"%s\"), TEXT(\"%s\"),\n\t\t\tTEXT(\"%s\"),\n\t\t\tFAxonActionHandler::CreateStatic(&F%sActions::%s),\n\t\t\tSchema);\n"),
			*Request.Namespace,
			*A.Name,
			*EscapedDesc,
			*ModuleName,
			*Handler);
		Out += TEXT("\t}\n\n");
	}
	Out += TEXT("}\n\n");

	for (const FAxonScaffoldActionSpec& A : Request.Actions)
	{
		const FString Handler = ToHandlerName(A.Name);
		Out += FString::Printf(
			TEXT("FAxonActionResult F%sActions::%s(const TSharedPtr<FJsonObject>& Params)\n{\n"),
			*ModuleName, *Handler);
		Out += FString::Printf(
			TEXT("\treturn FAxonActionResult::Error(\n\t\tFString::Printf(TEXT(\"not_implemented: %s.%s — scaffold stub. Research the target system and replace this handler.\")),\n\t\t%d);\n}\n\n"),
			*Request.Namespace, *A.Name, AxonScaffoldInternal::ErrNotImplemented);
	}

	return Out;
}

FString FAxonExtensionScaffolder::MakeModuleCpp(const FAxonScaffoldRequest& Request, const FString& ModuleName)
{
	FString Out;
	Out += FString::Printf(TEXT("#include \"%sModule.h\"\n"), *ModuleName);
	Out += FString::Printf(TEXT("#include \"%sActions.h\"\n"), *ModuleName);
	Out += TEXT("#include \"AxonCoreModule.h\"\n");
	Out += TEXT("#include \"AxonToolRegistry.h\"\n\n");
	Out += FString::Printf(TEXT("void F%sModule::StartupModule()\n{\n"), *ModuleName);
	if (Request.Actions.Num() > 0)
	{
		Out += FString::Printf(TEXT("\tF%sActions::RegisterAll();\n"), *ModuleName);
	}
	else
	{
		Out += TEXT("\t// skeleton_only: no actions registered yet\n");
	}
	Out += TEXT("}\n\n");
	Out += FString::Printf(TEXT("void F%sModule::ShutdownModule()\n{\n"), *ModuleName);
	Out += TEXT("\tif (FAxonCoreModule::IsAvailable())\n\t{\n");
	Out += FString::Printf(TEXT("\t\tFAxonToolRegistry::Get().UnregisterNamespace(TEXT(\"%s\"));\n"), *Request.Namespace);
	Out += TEXT("\t}\n}\n\n");
	Out += FString::Printf(TEXT("IMPLEMENT_MODULE(F%sModule, %s)\n"), *ModuleName, *ModuleName);
	return Out;
}

bool FAxonExtensionScaffolder::MergeUProjectEnablePlugin(const FString& PluginName, TArray<FString>& Written, FString& OutError, bool bDryRun)
{
	const FString UProjectPath = FPaths::GetProjectFilePath();
	FString Text;
	if (!FFileHelper::LoadFileToString(Text, *UProjectPath))
	{
		OutError = FString::Printf(TEXT("Failed to read uproject: %s"), *UProjectPath);
		return false;
	}

	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Text);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		OutError = TEXT("Failed to parse uproject JSON");
		return false;
	}

	TArray<TSharedPtr<FJsonValue>> PluginsArr;
	const TArray<TSharedPtr<FJsonValue>>* Existing = nullptr;
	if (Root->TryGetArrayField(TEXT("Plugins"), Existing) && Existing)
	{
		PluginsArr = *Existing;
	}

	for (const TSharedPtr<FJsonValue>& V : PluginsArr)
	{
		const TSharedPtr<FJsonObject>* Obj = nullptr;
		if (V->TryGetObject(Obj) && Obj && (*Obj)->GetStringField(TEXT("Name")).Equals(PluginName, ESearchCase::IgnoreCase))
		{
			(*Obj)->SetBoolField(TEXT("Enabled"), true);
			// already present
			if (!bDryRun)
			{
				FString OutText;
				TSharedRef<TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>> Writer =
					TJsonWriterFactory<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>::Create(&OutText);
				FJsonSerializer::Serialize(Root.ToSharedRef(), Writer);
				if (!FFileHelper::SaveStringToFile(OutText, *UProjectPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
				{
					OutError = TEXT("Failed to write uproject after enabling existing plugin entry");
					return false;
				}
				Written.Add(UProjectPath);
			}
			return true;
		}
	}

	TSharedPtr<FJsonObject> Entry = MakeShared<FJsonObject>();
	Entry->SetStringField(TEXT("Name"), PluginName);
	Entry->SetBoolField(TEXT("Enabled"), true);
	PluginsArr.Add(MakeShared<FJsonValueObject>(Entry));
	Root->SetArrayField(TEXT("Plugins"), PluginsArr);

	if (!bDryRun)
	{
		FString OutText;
		TSharedRef<TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>> Writer =
			TJsonWriterFactory<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>::Create(&OutText);
		FJsonSerializer::Serialize(Root.ToSharedRef(), Writer);
		if (!FFileHelper::SaveStringToFile(OutText, *UProjectPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
		{
			OutError = TEXT("Failed to write uproject");
			return false;
		}
		Written.Add(UProjectPath);
	}
	return true;
}

FAxonScaffoldResult FAxonExtensionScaffolder::Create(const FAxonScaffoldRequest& Request)
{
	FAxonScaffoldResult Result;

	if (!IsValidIdentifier(Request.PluginName, false))
	{
		Result.ErrorMessage = TEXT("plugin_name must be a valid PascalCase/alphanumeric identifier (e.g. AxonFoo)");
		return Result;
	}

	const FString Namespace = SanitizeNamespace(
		Request.Namespace.IsEmpty() ? Request.PluginName : Request.Namespace);
	if (Namespace.IsEmpty() || !IsValidIdentifier(Namespace, true) || FChar::IsDigit(Namespace[0]))
	{
		Result.ErrorMessage = TEXT("namespace must be lowercase snake_case starting with a letter (e.g. foo_bar)");
		return Result;
	}

	FAxonScaffoldRequest Effective = Request;
	Effective.Namespace = Namespace;

	if (Effective.Actions.Num() == 0 && !Effective.bSkeletonOnly)
	{
		Result.ErrorMessage =
			TEXT("actions is empty. Workflow: (1) ask the user if they want Action recommendations; ")
			TEXT("(2) if yes, call axon_research_extension_target and propose actions for confirmation; ")
			TEXT("(3) pass confirmed actions here. Or set skeleton_only=true for an empty shell.");
		return Result;
	}

	for (const FAxonScaffoldActionSpec& A : Effective.Actions)
	{
		if (A.Name.IsEmpty() || !IsValidIdentifier(A.Name, true))
		{
			Result.ErrorMessage = FString::Printf(TEXT("Invalid action name '%s' — use snake_case"), *A.Name);
			return Result;
		}
	}

	const FString ModuleName = Effective.PluginName;
	const FString PluginDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectPluginsDir() / Effective.PluginName);
	if (FPaths::DirectoryExists(PluginDir) || FPaths::FileExists(PluginDir / (Effective.PluginName + TEXT(".uplugin"))))
	{
		Result.ErrorMessage = FString::Printf(
			TEXT("Plugin directory already exists: %s — refuse to overwrite"), *PluginDir);
		return Result;
	}

	FString TmplErr;
	FString UpluginTmpl = LoadTemplate(TEXT("Plugin.uplugin.template"), TmplErr);
	if (UpluginTmpl.IsEmpty()) { Result.ErrorMessage = TmplErr; return Result; }
	FString BuildTmpl = LoadTemplate(TEXT("Module.Build.cs.template"), TmplErr);
	if (BuildTmpl.IsEmpty()) { Result.ErrorMessage = TmplErr; return Result; }
	FString ModuleHTmpl = LoadTemplate(TEXT("Module.h.template"), TmplErr);
	if (ModuleHTmpl.IsEmpty()) { Result.ErrorMessage = TmplErr; return Result; }
	FString ActionsHTmpl = LoadTemplate(TEXT("Actions.h.template"), TmplErr);
	if (ActionsHTmpl.IsEmpty()) { Result.ErrorMessage = TmplErr; return Result; }

	TMap<FString, FString> Vars;
	Vars.Add(TEXT("PluginName"), Effective.PluginName);
	Vars.Add(TEXT("ModuleName"), ModuleName);
	Vars.Add(TEXT("Namespace"), Effective.Namespace);
	Vars.Add(TEXT("FriendlyName"), Effective.PluginName);
	Vars.Add(TEXT("Description"),
		Effective.Intent.IsEmpty()
			? FString::Printf(TEXT("Axon sibling extension (%s)"), *Effective.Namespace)
			: Effective.Intent);
	Vars.Add(TEXT("DependPluginsJson"), MakeDependPluginsJson(Effective.DependPlugins));
	Vars.Add(TEXT("ExtraBuildModules"), MakeExtraBuildModules(Effective.BuildModules));
	Vars.Add(TEXT("ActionDeclarations"), MakeActionDeclarations(Effective.Actions, ModuleName));

	const FString UpluginBody = ApplyPlaceholders(UpluginTmpl, Vars);
	const FString BuildBody = ApplyPlaceholders(BuildTmpl, Vars);
	const FString ModuleHBody = ApplyPlaceholders(ModuleHTmpl, Vars);
	const FString ActionsHBody = ApplyPlaceholders(ActionsHTmpl, Vars);
	const FString ActionsCppBody = MakeActionsCpp(Effective, ModuleName);
	const FString ModuleCppBody = MakeModuleCpp(Effective, ModuleName);

	TArray<FString> PlannedPaths;
	PlannedPaths.Add(PluginDir / (Effective.PluginName + TEXT(".uplugin")));
	PlannedPaths.Add(PluginDir / TEXT("Source") / ModuleName / (ModuleName + TEXT(".Build.cs")));
	PlannedPaths.Add(PluginDir / TEXT("Source") / ModuleName / TEXT("Public") / (ModuleName + TEXT("Module.h")));
	PlannedPaths.Add(PluginDir / TEXT("Source") / ModuleName / TEXT("Private") / (ModuleName + TEXT("Module.cpp")));
	if (Effective.Actions.Num() > 0)
	{
		PlannedPaths.Add(PluginDir / TEXT("Source") / ModuleName / TEXT("Public") / (ModuleName + TEXT("Actions.h")));
		PlannedPaths.Add(PluginDir / TEXT("Source") / ModuleName / TEXT("Private") / (ModuleName + TEXT("Actions.cpp")));
	}
	PlannedPaths.Add(FPaths::GetProjectFilePath());

	TArray<TSharedPtr<FJsonValue>> ActionJson;
	for (const FAxonScaffoldActionSpec& A : Effective.Actions)
	{
		TSharedPtr<FJsonObject> O = MakeShared<FJsonObject>();
		O->SetStringField(TEXT("name"), A.Name);
		O->SetStringField(TEXT("description"), A.Description);
		ActionJson.Add(MakeShared<FJsonValueObject>(O));
	}

	TSharedPtr<FJsonObject> Payload = MakeShared<FJsonObject>();
	Payload->SetStringField(TEXT("plugin_name"), Effective.PluginName);
	Payload->SetStringField(TEXT("namespace"), Effective.Namespace);
	Payload->SetStringField(TEXT("plugin_path"), PluginDir);
	Payload->SetStringField(TEXT("mcp_tool"), Effective.Namespace + TEXT("_query"));
	Payload->SetBoolField(TEXT("dry_run"), Effective.bDryRun);
	Payload->SetBoolField(TEXT("skeleton_only"), Effective.bSkeletonOnly);
	Payload->SetArrayField(TEXT("actions"), ActionJson);

	TArray<TSharedPtr<FJsonValue>> FilesJson;
	for (const FString& P : PlannedPaths)
	{
		FilesJson.Add(MakeShared<FJsonValueString>(P));
	}
	Payload->SetArrayField(TEXT("files"), FilesJson);

	TArray<TSharedPtr<FJsonValue>> NextSteps;
	NextSteps.Add(MakeShared<FJsonValueString>(TEXT("Close the Unreal Editor")));
	NextSteps.Add(MakeShared<FJsonValueString>(TEXT("Compile with UBT (GameEditor Win64 Development)")));
	NextSteps.Add(MakeShared<FJsonValueString>(TEXT("Relaunch the editor")));
	NextSteps.Add(MakeShared<FJsonValueString>(
		FString::Printf(TEXT("Call axon_discover({\\\"namespace\\\":\\\"%s\\\"}) to verify actions"), *Effective.Namespace)));
	NextSteps.Add(MakeShared<FJsonValueString>(
		TEXT("Replace stub handlers (error -32020) with real implementations")));
	Payload->SetArrayField(TEXT("next_steps"), NextSteps);

	Payload->SetStringField(TEXT("workflow_reminder"),
		TEXT("Before recommending actions: ask the user; if they want recommendations, research the target system first, confirm the list, then scaffold."));

	if (Effective.bDryRun)
	{
		Payload->SetStringField(TEXT("status"), TEXT("dry_run"));
		Result.bSuccess = true;
		Result.Payload = Payload;
		return Result;
	}

	TArray<FString> Written;
	FString WriteErr;
	if (!WriteFile(PluginDir / (Effective.PluginName + TEXT(".uplugin")), UpluginBody, Written, WriteErr)
		|| !WriteFile(PluginDir / TEXT("Source") / ModuleName / (ModuleName + TEXT(".Build.cs")), BuildBody, Written, WriteErr)
		|| !WriteFile(PluginDir / TEXT("Source") / ModuleName / TEXT("Public") / (ModuleName + TEXT("Module.h")), ModuleHBody, Written, WriteErr)
		|| !WriteFile(PluginDir / TEXT("Source") / ModuleName / TEXT("Private") / (ModuleName + TEXT("Module.cpp")), ModuleCppBody, Written, WriteErr))
	{
		Result.ErrorMessage = WriteErr;
		return Result;
	}

	if (Effective.Actions.Num() > 0)
	{
		if (!WriteFile(PluginDir / TEXT("Source") / ModuleName / TEXT("Public") / (ModuleName + TEXT("Actions.h")), ActionsHBody, Written, WriteErr)
			|| !WriteFile(PluginDir / TEXT("Source") / ModuleName / TEXT("Private") / (ModuleName + TEXT("Actions.cpp")), ActionsCppBody, Written, WriteErr))
		{
			Result.ErrorMessage = WriteErr;
			return Result;
		}
	}

	if (!MergeUProjectEnablePlugin(Effective.PluginName, Written, WriteErr, false))
	{
		Result.ErrorMessage = WriteErr;
		return Result;
	}

	TArray<TSharedPtr<FJsonValue>> WrittenJson;
	for (const FString& P : Written)
	{
		WrittenJson.Add(MakeShared<FJsonValueString>(P));
	}
	Payload->SetArrayField(TEXT("written_files"), WrittenJson);
	Payload->SetStringField(TEXT("status"), TEXT("created"));

	Result.bSuccess = true;
	Result.Payload = Payload;
	return Result;
}

TSharedPtr<FJsonObject> FAxonExtensionScaffolder::ListRecipes(FString& OutError)
{
	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	TArray<TSharedPtr<FJsonValue>> Recipes;
	const FString Dir = GetRecipesDir();
	if (!FPaths::DirectoryExists(Dir))
	{
		Result->SetArrayField(TEXT("recipes"), Recipes);
		Result->SetStringField(TEXT("notes"), TEXT("No ExtensionRecipes directory — that is OK; recipes are optional and external."));
		return Result;
	}

	TArray<FString> Files;
	IFileManager::Get().FindFiles(Files, *(Dir / TEXT("*.json")), true, false);
	Files.Sort();
	for (const FString& File : Files)
	{
		if (File.StartsWith(TEXT("_")))
		{
			continue;
		}
		TSharedPtr<FJsonObject> Entry = MakeShared<FJsonObject>();
		Entry->SetStringField(TEXT("recipe_id"), FPaths::GetBaseFilename(File));
		Entry->SetStringField(TEXT("path"), Dir / File);
		Recipes.Add(MakeShared<FJsonValueObject>(Entry));
	}
	Result->SetArrayField(TEXT("recipes"), Recipes);
	Result->SetStringField(TEXT("notes"),
		TEXT("External recipes only. Core ships none. Recipes do not skip the ask→research→confirm workflow."));
	return Result;
}

bool FAxonExtensionScaffolder::LoadRecipe(const FString& RecipeId, FAxonScaffoldRequest& InOutRequest, FString& OutError)
{
	if (RecipeId.IsEmpty())
	{
		OutError = TEXT("recipe_id is empty");
		return false;
	}
	const FString Path = GetRecipesDir() / (RecipeId + TEXT(".json"));
	FString Text;
	if (!FFileHelper::LoadFileToString(Text, *Path))
	{
		OutError = FString::Printf(TEXT("Recipe not found: %s"), *Path);
		return false;
	}
	TSharedPtr<FJsonObject> Obj;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Text);
	if (!FJsonSerializer::Deserialize(Reader, Obj) || !Obj.IsValid())
	{
		OutError = TEXT("Failed to parse recipe JSON");
		return false;
	}

	FString Tmp;
	if (Obj->TryGetStringField(TEXT("plugin_name"), Tmp) && InOutRequest.PluginName.IsEmpty())
	{
		InOutRequest.PluginName = Tmp;
	}
	if (Obj->TryGetStringField(TEXT("namespace"), Tmp) && InOutRequest.Namespace.IsEmpty())
	{
		InOutRequest.Namespace = Tmp;
	}
	if (Obj->TryGetStringField(TEXT("intent"), Tmp) && InOutRequest.Intent.IsEmpty())
	{
		InOutRequest.Intent = Tmp;
	}

	const TArray<TSharedPtr<FJsonValue>>* Deps = nullptr;
	if (Obj->TryGetArrayField(TEXT("depend_plugins"), Deps) && Deps && InOutRequest.DependPlugins.Num() == 0)
	{
		for (const TSharedPtr<FJsonValue>& V : *Deps)
		{
			InOutRequest.DependPlugins.Add(V->AsString());
		}
	}
	const TArray<TSharedPtr<FJsonValue>>* Mods = nullptr;
	if (Obj->TryGetArrayField(TEXT("build_modules"), Mods) && Mods && InOutRequest.BuildModules.Num() == 0)
	{
		for (const TSharedPtr<FJsonValue>& V : *Mods)
		{
			InOutRequest.BuildModules.Add(V->AsString());
		}
	}
	const TArray<TSharedPtr<FJsonValue>>* Acts = nullptr;
	if (Obj->TryGetArrayField(TEXT("actions"), Acts) && Acts && InOutRequest.Actions.Num() == 0)
	{
		for (const TSharedPtr<FJsonValue>& V : *Acts)
		{
			const TSharedPtr<FJsonObject>* AObj = nullptr;
			if (!V->TryGetObject(AObj) || !AObj)
			{
				continue;
			}
			FAxonScaffoldActionSpec Spec;
			Spec.Name = (*AObj)->GetStringField(TEXT("name"));
			Spec.Description = (*AObj)->GetStringField(TEXT("description"));
			const TSharedPtr<FJsonObject>* Params = nullptr;
			if ((*AObj)->TryGetObjectField(TEXT("params"), Params) && Params)
			{
				Spec.ParamsSchema = *Params;
			}
			InOutRequest.Actions.Add(Spec);
		}
	}
	return true;
}
