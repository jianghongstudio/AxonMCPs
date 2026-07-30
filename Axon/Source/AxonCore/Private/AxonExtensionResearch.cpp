#include "AxonExtensionResearch.h"
#include "AxonJsonUtils.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "HAL/FileManager.h"

namespace AxonResearchInternal
{
	static FString ResolvePluginDir(const FString& TargetPlugin, FString& OutError)
	{
		if (TargetPlugin.IsEmpty())
		{
			OutError = TEXT("target_plugin is required — plugin name or filesystem path");
			return FString();
		}

		// Absolute or project-relative path to a directory
		FString AsPath = TargetPlugin;
		if (FPaths::DirectoryExists(AsPath))
		{
			return FPaths::ConvertRelativePathToFull(AsPath);
		}

		// Prefer Axon MCP extensions root (Plugins/AxonMCPs/<name>).
		const FString AxonMcpsPlugin = FPaths::Combine(FPaths::ProjectPluginsDir(), TEXT("AxonMCPs"), TargetPlugin);
		if (FPaths::DirectoryExists(AxonMcpsPlugin))
		{
			return FPaths::ConvertRelativePathToFull(AxonMcpsPlugin);
		}

		const FString ProjectPlugins = FPaths::Combine(FPaths::ProjectPluginsDir(), TargetPlugin);
		if (FPaths::DirectoryExists(ProjectPlugins))
		{
			return FPaths::ConvertRelativePathToFull(ProjectPlugins);
		}

		TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TargetPlugin);
		if (Plugin.IsValid())
		{
			return Plugin->GetBaseDir();
		}

		// Fuzzy: search AxonMCPs first, then ProjectPluginsDir children
		auto FindNamedChild = [&](const FString& ParentDir) -> FString
		{
			TArray<FString> Dirs;
			IFileManager::Get().FindFiles(Dirs, *(ParentDir / TEXT("*")), false, true);
			for (const FString& DirName : Dirs)
			{
				if (DirName.Equals(TargetPlugin, ESearchCase::IgnoreCase))
				{
					return FPaths::ConvertRelativePathToFull(ParentDir / DirName);
				}
			}
			return FString();
		};

		if (FString Hit = FindNamedChild(FPaths::ProjectPluginsDir() / TEXT("AxonMCPs")); !Hit.IsEmpty())
		{
			return Hit;
		}
		if (FString Hit = FindNamedChild(FPaths::ProjectPluginsDir()); !Hit.IsEmpty())
		{
			return Hit;
		}

		OutError = FString::Printf(
			TEXT("Could not resolve target_plugin '%s' — pass an installed plugin name or an existing directory path"),
			*TargetPlugin);
		return FString();
	}

	static TSharedPtr<FJsonObject> ParseUplugin(const FString& PluginDir)
	{
		TArray<FString> Uplugins;
		IFileManager::Get().FindFiles(Uplugins, *(PluginDir / TEXT("*.uplugin")), true, false);
		if (Uplugins.Num() == 0)
		{
			return nullptr;
		}

		FString JsonText;
		if (!FFileHelper::LoadFileToString(JsonText, *(PluginDir / Uplugins[0])))
		{
			return nullptr;
		}

		TSharedPtr<FJsonObject> Obj;
		const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
		if (!FJsonSerializer::Deserialize(Reader, Obj) || !Obj.IsValid())
		{
			return nullptr;
		}
		return Obj;
	}

	static void CollectHeaders(const FString& Root, TArray<TSharedPtr<FJsonValue>>& OutHeaders, int32 MaxCount)
	{
		TArray<FString> Files;
		IFileManager::Get().FindFilesRecursive(Files, *Root, TEXT("*.h"), true, false);
		Files.Sort();
		for (const FString& File : Files)
		{
			if (OutHeaders.Num() >= MaxCount)
			{
				break;
			}
			TSharedPtr<FJsonObject> H = MakeShared<FJsonObject>();
			H->SetStringField(TEXT("path"), File);
			H->SetStringField(TEXT("relative"), File.RightChop(Root.Len()).Replace(TEXT("\\"), TEXT("/")));
			OutHeaders.Add(MakeShared<FJsonValueObject>(H));
		}
	}

	static void CollectDocs(const FString& PluginDir, TArray<TSharedPtr<FJsonValue>>& OutDocs, int32 MaxFiles, int32 MaxChars)
	{
		TArray<FString> Candidates;
		const FString Readme = PluginDir / TEXT("README.md");
		if (FPaths::FileExists(Readme))
		{
			Candidates.Add(Readme);
		}

		const FString DocsDir = PluginDir / TEXT("Docs");
		if (FPaths::DirectoryExists(DocsDir))
		{
			TArray<FString> Md;
			IFileManager::Get().FindFilesRecursive(Md, *DocsDir, TEXT("*.md"), true, false);
			Md.Sort();
			Candidates.Append(Md);
		}

		const FString Knowledges = PluginDir / TEXT(".Knowledges");
		if (FPaths::DirectoryExists(Knowledges))
		{
			TArray<FString> Md;
			IFileManager::Get().FindFilesRecursive(Md, *Knowledges, TEXT("*.md"), true, false);
			Md.Sort();
			Candidates.Append(Md);
		}

		int32 Count = 0;
		for (const FString& Path : Candidates)
		{
			if (Count >= MaxFiles)
			{
				break;
			}
			FString Text;
			if (!FFileHelper::LoadFileToString(Text, *Path))
			{
				continue;
			}
			if (Text.Len() > MaxChars)
			{
				Text = Text.Left(MaxChars) + TEXT("\n...[truncated]");
			}
			TSharedPtr<FJsonObject> Doc = MakeShared<FJsonObject>();
			Doc->SetStringField(TEXT("path"), Path);
			Doc->SetStringField(TEXT("excerpt"), Text);
			OutDocs.Add(MakeShared<FJsonValueObject>(Doc));
			++Count;
		}
	}
}

TSharedPtr<FJsonObject> FAxonExtensionResearch::Research(const FString& TargetPlugin, const TArray<FString>& ExtraPaths, FString& OutError)
{
	using namespace AxonResearchInternal;

	const FString PluginDir = ResolvePluginDir(TargetPlugin, OutError);
	if (PluginDir.IsEmpty())
	{
		return nullptr;
	}

	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetStringField(TEXT("resolved_path"), PluginDir);
	Result->SetStringField(TEXT("target_plugin"), TargetPlugin);

	TArray<TSharedPtr<FJsonValue>> ModulesArr;
	TArray<TSharedPtr<FJsonValue>> DependPluginsArr;
	TSharedPtr<FJsonObject> Uplugin = ParseUplugin(PluginDir);
	if (Uplugin.IsValid())
	{
		Result->SetStringField(TEXT("friendly_name"), Uplugin->GetStringField(TEXT("FriendlyName")));
		Result->SetStringField(TEXT("description"), Uplugin->GetStringField(TEXT("Description")));

		const TArray<TSharedPtr<FJsonValue>>* ModulesJson = nullptr;
		if (Uplugin->TryGetArrayField(TEXT("Modules"), ModulesJson) && ModulesJson)
		{
			for (const TSharedPtr<FJsonValue>& V : *ModulesJson)
			{
				const TSharedPtr<FJsonObject>* ModObj = nullptr;
				if (!V->TryGetObject(ModObj) || !ModObj)
				{
					continue;
				}
				TSharedPtr<FJsonObject> M = MakeShared<FJsonObject>();
				M->SetStringField(TEXT("name"), (*ModObj)->GetStringField(TEXT("Name")));
				M->SetStringField(TEXT("type"), (*ModObj)->GetStringField(TEXT("Type")));
				M->SetStringField(TEXT("loading_phase"), (*ModObj)->GetStringField(TEXT("LoadingPhase")));
				ModulesArr.Add(MakeShared<FJsonValueObject>(M));
			}
		}

		const TArray<TSharedPtr<FJsonValue>>* PluginsJson = nullptr;
		if (Uplugin->TryGetArrayField(TEXT("Plugins"), PluginsJson) && PluginsJson)
		{
			for (const TSharedPtr<FJsonValue>& V : *PluginsJson)
			{
				const TSharedPtr<FJsonObject>* PObj = nullptr;
				if (!V->TryGetObject(PObj) || !PObj)
				{
					continue;
				}
				DependPluginsArr.Add(MakeShared<FJsonValueString>((*PObj)->GetStringField(TEXT("Name"))));
			}
		}
	}

	// Also discover Source/* module folders
	const FString SourceDir = PluginDir / TEXT("Source");
	if (FPaths::DirectoryExists(SourceDir))
	{
		TArray<FString> ModDirs;
		IFileManager::Get().FindFiles(ModDirs, *(SourceDir / TEXT("*")), false, true);
		TSet<FString> Seen;
		for (const TSharedPtr<FJsonValue>& Existing : ModulesArr)
		{
			Seen.Add(Existing->AsObject()->GetStringField(TEXT("name")));
		}
		for (const FString& ModName : ModDirs)
		{
			if (Seen.Contains(ModName))
			{
				continue;
			}
			TSharedPtr<FJsonObject> M = MakeShared<FJsonObject>();
			M->SetStringField(TEXT("name"), ModName);
			M->SetStringField(TEXT("type"), TEXT("unknown"));
			M->SetStringField(TEXT("loading_phase"), TEXT(""));
			M->SetBoolField(TEXT("from_source_dir"), true);
			ModulesArr.Add(MakeShared<FJsonValueObject>(M));
		}
	}
	Result->SetArrayField(TEXT("modules"), ModulesArr);
	Result->SetArrayField(TEXT("depend_plugins"), DependPluginsArr);

	TArray<TSharedPtr<FJsonValue>> Headers;
	if (FPaths::DirectoryExists(SourceDir))
	{
		TArray<FString> ModDirs;
		IFileManager::Get().FindFiles(ModDirs, *(SourceDir / TEXT("*")), false, true);
		for (const FString& ModName : ModDirs)
		{
			const FString PublicDir = SourceDir / ModName / TEXT("Public");
			if (FPaths::DirectoryExists(PublicDir))
			{
				CollectHeaders(PublicDir, Headers, 80);
			}
		}
	}
	for (const FString& Extra : ExtraPaths)
	{
		if (FPaths::DirectoryExists(Extra))
		{
			CollectHeaders(Extra, Headers, 120);
		}
	}
	Result->SetArrayField(TEXT("public_headers"), Headers);

	TArray<TSharedPtr<FJsonValue>> Docs;
	CollectDocs(PluginDir, Docs, 8, 4000);
	for (const FString& Extra : ExtraPaths)
	{
		if (FPaths::DirectoryExists(Extra))
		{
			CollectDocs(Extra, Docs, 12, 4000);
		}
	}
	Result->SetArrayField(TEXT("docs"), Docs);

	Result->SetStringField(TEXT("notes"),
		TEXT("Domain-agnostic scan only. Do NOT treat this as an Action recommendation. ")
		TEXT("If the user asked for recommendations: research findings first, propose actions, get confirmation, then call axon_create_extension."));

	return Result;
}
