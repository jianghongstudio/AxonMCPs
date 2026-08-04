#include "AxonKnowledgeScaffold.h"
#include "Misc/App.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FString FAxonKnowledgeScaffold::ToSnakeCase(const FString& PascalOrMixed)
{
	FString Out;
	Out.Reserve(PascalOrMixed.Len() * 2);
	for (int32 i = 0; i < PascalOrMixed.Len(); ++i)
	{
		const TCHAR Ch = PascalOrMixed[i];
		if (Ch == TEXT('-') || Ch == TEXT(' ') || Ch == TEXT('_'))
		{
			if (Out.Len() > 0 && !Out.EndsWith(TEXT("_")))
			{
				Out.AppendChar(TEXT('_'));
			}
			continue;
		}
		if (!FChar::IsAlnum(Ch))
		{
			continue;
		}
		if (i > 0 && FChar::IsUpper(Ch))
		{
			const TCHAR Prev = PascalOrMixed[i - 1];
			const bool bPrevLowerOrDigit = FChar::IsLower(Prev) || FChar::IsDigit(Prev);
			const bool bNextLower = (i + 1 < PascalOrMixed.Len()) && FChar::IsLower(PascalOrMixed[i + 1]);
			if (bPrevLowerOrDigit || (FChar::IsUpper(Prev) && bNextLower))
			{
				if (!Out.EndsWith(TEXT("_")))
				{
					Out.AppendChar(TEXT('_'));
				}
			}
		}
		Out.AppendChar(FChar::ToLower(Ch));
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

FString FAxonKnowledgeScaffold::SanitizePluginToken(const FString& In)
{
	FString Out;
	for (TCHAR Ch : In)
	{
		if (FChar::IsAlnum(Ch))
		{
			Out.AppendChar(Ch);
		}
	}
	return Out;
}

bool FAxonKnowledgeScaffold::IsValidIdentifier(const FString& Name)
{
	if (Name.IsEmpty() || !(FChar::IsAlpha(Name[0]) || Name[0] == TEXT('_')))
	{
		return false;
	}
	for (TCHAR Ch : Name)
	{
		if (!(FChar::IsAlnum(Ch) || Ch == TEXT('_')))
		{
			return false;
		}
	}
	return true;
}

FAxonKbNamePlan FAxonKnowledgeScaffold::DeriveNames(const FString& ProjectName)
{
	FAxonKbNamePlan Plan;
	Plan.ProjectName = ProjectName.IsEmpty() ? FString(FApp::GetProjectName()) : ProjectName;
	const FString Token = SanitizePluginToken(Plan.ProjectName);
	Plan.PluginName = FString::Printf(TEXT("Axon%sKB"), *Token);
	Plan.Namespace = ToSnakeCase(Token) + TEXT("_kb");
	Plan.McpTool = Plan.Namespace + TEXT("_query");
	Plan.PluginPath = FPaths::ConvertRelativePathToFull(
		FPaths::Combine(FPaths::ProjectPluginsDir(), TEXT("AxonMCPs"), Plan.PluginName));
	return Plan;
}

bool FAxonKnowledgeScaffold::WriteFile(const FString& Path, const FString& Contents, TArray<FString>& Written, FString& OutError)
{
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(Path), true);
	if (!FFileHelper::SaveStringToFile(Contents, *Path, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{
		OutError = FString::Printf(TEXT("Failed to write: %s"), *Path);
		return false;
	}
	Written.Add(Path);
	return true;
}

bool FAxonKnowledgeScaffold::MergeUProjectEnablePlugin(const FString& PluginName, TArray<FString>& Written, FString& OutError, bool bDryRun)
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

FString FAxonKnowledgeScaffold::MakeUplugin(const FString& PluginName, const FString& Namespace)
{
	return FString::Printf(
		TEXT("{\n")
		TEXT("\t\"FileVersion\": 3,\n")
		TEXT("\t\"Version\": 1,\n")
		TEXT("\t\"VersionName\": \"0.1.0\",\n")
		TEXT("\t\"FriendlyName\": \"%s\",\n")
		TEXT("\t\"Description\": \"Offline project knowledge corpus (namespace %s). Thin shell over AxonKnowledgeLib.\",\n")
		TEXT("\t\"Category\": \"Editor\",\n")
		TEXT("\t\"CreatedBy\": \"AxonKnowledgeLib scaffold_kb_plugin\",\n")
		TEXT("\t\"EnabledByDefault\": true,\n")
		TEXT("\t\"Installed\": false,\n")
		TEXT("\t\"Plugins\": [\n")
		TEXT("\t\t{ \"Name\": \"Axon\", \"Enabled\": true },\n")
		TEXT("\t\t{ \"Name\": \"AxonKnowledgeLib\", \"Enabled\": true }\n")
		TEXT("\t],\n")
		TEXT("\t\"Modules\": [\n")
		TEXT("\t\t{\n")
		TEXT("\t\t\t\"Name\": \"%s\",\n")
		TEXT("\t\t\t\"Type\": \"Editor\",\n")
		TEXT("\t\t\t\"LoadingPhase\": \"PostEngineInit\"\n")
		TEXT("\t\t}\n")
		TEXT("\t]\n")
		TEXT("}\n"),
		*PluginName, *Namespace, *PluginName);
}

FString FAxonKnowledgeScaffold::MakeBuildCs(const FString& ModuleName)
{
	return FString::Printf(
		TEXT("using UnrealBuildTool;\n\n")
		TEXT("public class %s : ModuleRules\n")
		TEXT("{\n")
		TEXT("\tpublic %s(ReadOnlyTargetRules Target) : base(Target)\n")
		TEXT("\t{\n")
		TEXT("\t\tPCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;\n\n")
		TEXT("\t\tPublicDependencyModuleNames.AddRange(new string[]\n")
		TEXT("\t\t{\n")
		TEXT("\t\t\t\"Core\",\n")
		TEXT("\t\t\t\"CoreUObject\",\n")
		TEXT("\t\t\t\"Engine\",\n")
		TEXT("\t\t\t\"UnrealEd\",\n")
		TEXT("\t\t\t\"Json\",\n")
		TEXT("\t\t\t\"Projects\",\n")
		TEXT("\t\t\t\"AxonCore\",\n")
		TEXT("\t\t\t\"AxonKnowledgeLib\"\n")
		TEXT("\t\t});\n")
		TEXT("\t}\n")
		TEXT("}\n"),
		*ModuleName, *ModuleName);
}

FString FAxonKnowledgeScaffold::MakeModuleH(const FString& ModuleName)
{
	return FString::Printf(
		TEXT("#pragma once\n\n")
		TEXT("#include \"Modules/ModuleManager.h\"\n\n")
		TEXT("class F%sModule : public IModuleInterface\n")
		TEXT("{\n")
		TEXT("public:\n")
		TEXT("\tvirtual void StartupModule() override;\n")
		TEXT("\tvirtual void ShutdownModule() override;\n")
		TEXT("};\n"),
		*ModuleName);
}

FString FAxonKnowledgeScaffold::MakeModuleCpp(const FString& ModuleName, const FString& Namespace, const FString& PluginName)
{
	return FString::Printf(
		TEXT("#include \"%sModule.h\"\n")
		TEXT("#include \"AxonKnowledgeRegistration.h\"\n")
		TEXT("#include \"AxonCoreModule.h\"\n")
		TEXT("#include \"AxonToolRegistry.h\"\n\n")
		TEXT("void F%sModule::StartupModule()\n")
		TEXT("{\n")
		TEXT("\tFAxonKnowledgeRegistration::RegisterAll(TEXT(\"%s\"), TEXT(\"%s\"));\n")
		TEXT("}\n\n")
		TEXT("void F%sModule::ShutdownModule()\n")
		TEXT("{\n")
		TEXT("\tif (FAxonCoreModule::IsAvailable())\n")
		TEXT("\t{\n")
		TEXT("\t\tFAxonToolRegistry::Get().UnregisterNamespace(TEXT(\"%s\"));\n")
		TEXT("\t}\n")
		TEXT("}\n\n")
		TEXT("IMPLEMENT_MODULE(F%sModule, %s)\n"),
		*ModuleName, *ModuleName, *Namespace, *PluginName, *ModuleName, *Namespace, *ModuleName, *ModuleName);
}

FString FAxonKnowledgeScaffold::MakeRoutingMd(const FString& ProjectName, const FString& Namespace)
{
	return FString::Printf(
		TEXT("# Routing — %s\n\n")
		TEXT("> Offline knowledge pack for project **%s** (MCP `%s_query`).\n\n")
		TEXT("## Question → Doc\n\n")
		TEXT("| Question / symptom | Doc | Notes |\n")
		TEXT("|---|---|---|\n")
		TEXT("| (fill after distill) | 01-architecture.md | |\n\n")
		TEXT("Replace this stub after extract + markdown distillation.\n"),
		*ProjectName, *ProjectName, *Namespace);
}

FString FAxonKnowledgeScaffold::MakeReadmeMd(const FString& PluginName, const FString& Namespace, const FString& ProjectName)
{
	return FString::Printf(
		TEXT("# %s\n\n")
		TEXT("Standalone offline knowledge plugin for **%s**.\n\n")
		TEXT("- Namespace: `%s`\n")
		TEXT("- MCP tool: `%s_query`\n")
		TEXT("- Query actions: `route` / `search` / `read` / `list_topics`\n")
		TEXT("- Extract actions write into `Knowledge/_raw/`\n")
		TEXT("- Depends on `AxonKnowledgeLib` (no business corpus in the lib)\n\n")
		TEXT("## Distill workflow\n\n")
		TEXT("1. `knowledge_query` → `scaffold_kb_plugin` (this plugin)\n")
		TEXT("2. Close editor → UBT → relaunch\n")
		TEXT("3. `%s_query` extract_* / extract_bundle\n")
		TEXT("4. Agent distills `Knowledge/*.md` from `_raw`\n")
		TEXT("5. Accept with search/read\n"),
		*PluginName, *ProjectName, *Namespace, *Namespace, *Namespace);
}

FAxonKbScaffoldResult FAxonKnowledgeScaffold::Create(const FAxonKbScaffoldRequest& Request)
{
	FAxonKbScaffoldResult Result;
	FAxonKbNamePlan Plan = DeriveNames(Request.ProjectName);
	if (!Request.PluginName.IsEmpty())
	{
		Plan.PluginName = Request.PluginName;
		Plan.PluginPath = FPaths::ConvertRelativePathToFull(
			FPaths::Combine(FPaths::ProjectPluginsDir(), TEXT("AxonMCPs"), Plan.PluginName));
	}
	if (!Request.Namespace.IsEmpty())
	{
		Plan.Namespace = Request.Namespace;
		Plan.McpTool = Plan.Namespace + TEXT("_query");
	}

	if (!IsValidIdentifier(Plan.PluginName))
	{
		Result.ErrorMessage = TEXT("plugin_name must be a valid identifier (e.g. AxonFooKB)");
		return Result;
	}
	if (!IsValidIdentifier(Plan.Namespace))
	{
		Result.ErrorMessage = TEXT("namespace must be a valid snake_case identifier (e.g. foo_kb)");
		return Result;
	}

	const FString ModuleName = Plan.PluginName;
	const FString PluginDir = Plan.PluginPath;
	const bool bExists = FPaths::DirectoryExists(PluginDir);

	TSharedPtr<FJsonObject> Payload = MakeShared<FJsonObject>();
	Payload->SetStringField(TEXT("project_name"), Plan.ProjectName);
	Payload->SetStringField(TEXT("plugin_name"), Plan.PluginName);
	Payload->SetStringField(TEXT("namespace"), Plan.Namespace);
	Payload->SetStringField(TEXT("mcp_tool"), Plan.McpTool);
	Payload->SetStringField(TEXT("plugin_path"), PluginDir);
	Payload->SetBoolField(TEXT("dry_run"), Request.bDryRun);
	Payload->SetBoolField(TEXT("already_exists"), bExists);

	TArray<FString> Planned;
	Planned.Add(PluginDir / (Plan.PluginName + TEXT(".uplugin")));
	Planned.Add(PluginDir / TEXT("Source") / ModuleName / (ModuleName + TEXT(".Build.cs")));
	Planned.Add(PluginDir / TEXT("Source") / ModuleName / TEXT("Public") / (ModuleName + TEXT("Module.h")));
	Planned.Add(PluginDir / TEXT("Source") / ModuleName / TEXT("Private") / (ModuleName + TEXT("Module.cpp")));
	Planned.Add(PluginDir / TEXT("Knowledge") / TEXT("00-routing.md"));
	Planned.Add(PluginDir / TEXT("Knowledge") / TEXT("README.md"));
	Planned.Add(PluginDir / TEXT("Knowledge") / TEXT("_raw") / TEXT(".gitkeep"));
	Planned.Add(FPaths::GetProjectFilePath());

	TArray<TSharedPtr<FJsonValue>> FilesJson;
	for (const FString& P : Planned)
	{
		FilesJson.Add(MakeShared<FJsonValueString>(P));
	}
	Payload->SetArrayField(TEXT("files"), FilesJson);

	TArray<TSharedPtr<FJsonValue>> NextSteps;
	NextSteps.Add(MakeShared<FJsonValueString>(TEXT("Close the Unreal Editor")));
	NextSteps.Add(MakeShared<FJsonValueString>(TEXT("Compile with UBT (ProjectEditor Win64 Development)")));
	NextSteps.Add(MakeShared<FJsonValueString>(TEXT("Relaunch the editor")));
	NextSteps.Add(MakeShared<FJsonValueString>(
		FString::Printf(TEXT("Call axon_discover({\\\"namespace\\\":\\\"%s\\\"})"), *Plan.Namespace)));
	NextSteps.Add(MakeShared<FJsonValueString>(
		FString::Printf(TEXT("Run %s_query extract_* / extract_bundle into Knowledge/_raw"), *Plan.Namespace)));
	NextSteps.Add(MakeShared<FJsonValueString>(TEXT("Agent-distill Knowledge/*.md from _raw evidence")));
	NextSteps.Add(MakeShared<FJsonValueString>(
		FString::Printf(TEXT("Accept with %s_query search/read"), *Plan.Namespace)));
	Payload->SetArrayField(TEXT("next_steps"), NextSteps);

	if (bExists && !Request.bForce && !Request.bDryRun)
	{
		Result.ErrorMessage = FString::Printf(
			TEXT("Plugin directory already exists: %s (pass force=true to overwrite scaffolding files)"),
			*PluginDir);
		Result.Payload = Payload;
		return Result;
	}

	if (Request.bDryRun)
	{
		Payload->SetStringField(TEXT("status"), TEXT("dry_run"));
		Result.bSuccess = true;
		Result.Payload = Payload;
		return Result;
	}

	TArray<FString> Written;
	FString WriteErr;
	if (!WriteFile(PluginDir / (Plan.PluginName + TEXT(".uplugin")), MakeUplugin(Plan.PluginName, Plan.Namespace), Written, WriteErr)
		|| !WriteFile(PluginDir / TEXT("Source") / ModuleName / (ModuleName + TEXT(".Build.cs")), MakeBuildCs(ModuleName), Written, WriteErr)
		|| !WriteFile(PluginDir / TEXT("Source") / ModuleName / TEXT("Public") / (ModuleName + TEXT("Module.h")), MakeModuleH(ModuleName), Written, WriteErr)
		|| !WriteFile(PluginDir / TEXT("Source") / ModuleName / TEXT("Private") / (ModuleName + TEXT("Module.cpp")), MakeModuleCpp(ModuleName, Plan.Namespace, Plan.PluginName), Written, WriteErr)
		|| !WriteFile(PluginDir / TEXT("Knowledge") / TEXT("00-routing.md"), MakeRoutingMd(Plan.ProjectName, Plan.Namespace), Written, WriteErr)
		|| !WriteFile(PluginDir / TEXT("Knowledge") / TEXT("README.md"), MakeReadmeMd(Plan.PluginName, Plan.Namespace, Plan.ProjectName), Written, WriteErr)
		|| !WriteFile(PluginDir / TEXT("Knowledge") / TEXT("_raw") / TEXT(".gitkeep"), TEXT(""), Written, WriteErr))
	{
		Result.ErrorMessage = WriteErr;
		return Result;
	}

	// Also enable AxonKnowledgeLib in uproject if missing
	if (!MergeUProjectEnablePlugin(TEXT("AxonKnowledgeLib"), Written, WriteErr, false)
		|| !MergeUProjectEnablePlugin(Plan.PluginName, Written, WriteErr, false))
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
