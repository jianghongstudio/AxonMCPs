#include "AxonKnowledgeExtract.h"
#include "AxonKnowledgeCorpus.h"
#include "AxonJsonUtils.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

FString FAxonKnowledgeExtract::SanitizeAssetFileStem(const FString& AssetPath)
{
	FString Stem = AssetPath;
	Stem.ReplaceInline(TEXT("/Game/"), TEXT(""));
	Stem.ReplaceInline(TEXT("/"), TEXT("__"));
	Stem.ReplaceInline(TEXT("\\"), TEXT("__"));
	Stem.ReplaceInline(TEXT("."), TEXT("_"));
	Stem.ReplaceInline(TEXT(" "), TEXT("_"));
	while (Stem.Contains(TEXT("____")))
	{
		Stem.ReplaceInline(TEXT("____"), TEXT("__"));
	}
	return Stem;
}

bool FAxonKnowledgeExtract::WriteJsonToRaw(
	const FString& PluginName,
	const FString& RelativePathUnderRaw,
	const TSharedPtr<FJsonObject>& Json,
	FString& OutAbsPath,
	FString& OutError)
{
	if (!Json.IsValid())
	{
		OutError = TEXT("json payload is null");
		return false;
	}

	FString Rel = RelativePathUnderRaw;
	Rel.ReplaceInline(TEXT("\\"), TEXT("/"));
	while (Rel.StartsWith(TEXT("/")))
	{
		Rel.RightChopInline(1);
	}
	if (Rel.Contains(TEXT("..")))
	{
		OutError = TEXT("path must not contain '..'");
		return false;
	}
	if (Rel.IsEmpty())
	{
		OutError = TEXT("path is empty");
		return false;
	}

	const FString RawRoot = FAxonKnowledgeCorpus::Get(PluginName).GetRawRoot();
	OutAbsPath = FPaths::ConvertRelativePathToFull(FPaths::Combine(RawRoot, Rel));
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(OutAbsPath), true);

	const FString Text = FAxonJsonUtils::Serialize(Json);
	if (!FFileHelper::SaveStringToFile(Text, *OutAbsPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{
		OutError = FString::Printf(TEXT("Failed to write: %s"), *OutAbsPath);
		return false;
	}
	return true;
}

FAxonActionResult FAxonKnowledgeExtract::InvokeAndWrite(
	const FString& PluginName,
	const FString& TargetNamespace,
	const FString& TargetAction,
	const TSharedPtr<FJsonObject>& Params,
	const FString& RelativePathUnderRaw)
{
	FAxonActionResult Invoked = FAxonToolRegistry::Get().ExecuteAction(TargetNamespace, TargetAction, Params);
	if (!Invoked.bSuccess)
	{
		return Invoked;
	}

	TSharedPtr<FJsonObject> Payload = Invoked.Result;
	if (!Payload.IsValid())
	{
		Payload = MakeShared<FJsonObject>();
		Payload->SetBoolField(TEXT("ok"), true);
	}

	FString AbsPath;
	FString Err;
	if (!WriteJsonToRaw(PluginName, RelativePathUnderRaw, Payload, AbsPath, Err))
	{
		return FAxonActionResult::Error(Err);
	}

	TSharedPtr<FJsonObject> Out = MakeShared<FJsonObject>();
	Out->SetStringField(TEXT("written"), AbsPath);
	Out->SetStringField(TEXT("path"), RelativePathUnderRaw);
	Out->SetStringField(TEXT("target_namespace"), TargetNamespace);
	Out->SetStringField(TEXT("target_action"), TargetAction);
	Out->SetObjectField(TEXT("result"), Payload);
	return FAxonActionResult::Success(Out);
}

FAxonActionResult FAxonKnowledgeExtract::HandleExtractWrite(const FString& PluginName, const TSharedPtr<FJsonObject>& Params)
{
	if (!Params.IsValid())
	{
		return FAxonActionResult::Error(TEXT("Missing params"));
	}
	FString Path;
	if (!Params->TryGetStringField(TEXT("path"), Path) || Path.IsEmpty())
	{
		return FAxonActionResult::Error(TEXT("Missing required parameter: path"));
	}
	const TSharedPtr<FJsonObject>* JsonObj = nullptr;
	if (!Params->TryGetObjectField(TEXT("json"), JsonObj) || !JsonObj || !JsonObj->IsValid())
	{
		return FAxonActionResult::Error(TEXT("Missing required parameter: json (object)"));
	}

	FString AbsPath;
	FString Err;
	if (!WriteJsonToRaw(PluginName, Path, *JsonObj, AbsPath, Err))
	{
		return FAxonActionResult::Error(Err);
	}
	TSharedPtr<FJsonObject> Out = MakeShared<FJsonObject>();
	Out->SetStringField(TEXT("written"), AbsPath);
	Out->SetStringField(TEXT("path"), Path);
	return FAxonActionResult::Success(Out);
}

FAxonActionResult FAxonKnowledgeExtract::HandleExtractInvoke(const FString& PluginName, const TSharedPtr<FJsonObject>& Params)
{
	if (!Params.IsValid())
	{
		return FAxonActionResult::Error(TEXT("Missing params"));
	}
	FString Ns, Action, Path;
	if (!Params->TryGetStringField(TEXT("target_namespace"), Ns) || Ns.IsEmpty())
	{
		return FAxonActionResult::Error(TEXT("Missing required parameter: target_namespace"));
	}
	if (!Params->TryGetStringField(TEXT("target_action"), Action) || Action.IsEmpty())
	{
		return FAxonActionResult::Error(TEXT("Missing required parameter: target_action"));
	}
	if (!Params->TryGetStringField(TEXT("path"), Path) || Path.IsEmpty())
	{
		return FAxonActionResult::Error(TEXT("Missing required parameter: path"));
	}

	TSharedPtr<FJsonObject> Fwd = MakeShared<FJsonObject>();
	const TSharedPtr<FJsonObject>* Nested = nullptr;
	if (Params->TryGetObjectField(TEXT("params"), Nested) && Nested && Nested->IsValid())
	{
		Fwd = *Nested;
	}

	return InvokeAndWrite(PluginName, Ns, Action, Fwd, Path);
}

FAxonActionResult FAxonKnowledgeExtract::HandleExtractStateMachines(const FString& PluginName, const TSharedPtr<FJsonObject>& Params)
{
	if (!Params.IsValid())
	{
		return FAxonActionResult::Error(TEXT("Missing params"));
	}
	FString AssetPath;
	if (!Params->TryGetStringField(TEXT("asset_path"), AssetPath) || AssetPath.IsEmpty())
	{
		return FAxonActionResult::Error(TEXT("Missing required parameter: asset_path"));
	}
	FString Label;
	if (!Params->TryGetStringField(TEXT("label"), Label) || Label.IsEmpty())
	{
		Label = SanitizeAssetFileStem(AssetPath);
	}
	bool bIncludeTransitions = true;
	Params->TryGetBoolField(TEXT("include_transitions"), bIncludeTransitions);

	TSharedPtr<FJsonObject> SmParams = MakeShared<FJsonObject>();
	SmParams->SetStringField(TEXT("asset_path"), AssetPath);
	const FString SmRel = FString::Printf(TEXT("abp/%s/state_machines.json"), *Label);
	FAxonActionResult SmResult = InvokeAndWrite(PluginName, TEXT("animation"), TEXT("get_state_machines"), SmParams, SmRel);
	if (!SmResult.bSuccess)
	{
		return SmResult;
	}

	TSharedPtr<FJsonObject> Out = MakeShared<FJsonObject>();
	TArray<TSharedPtr<FJsonValue>> Written;
	Written.Add(MakeShared<FJsonValueString>(SmResult.Result->GetStringField(TEXT("written"))));

	if (bIncludeTransitions && SmResult.Result.IsValid())
	{
		const TSharedPtr<FJsonObject>* NestedResult = nullptr;
		if (SmResult.Result->TryGetObjectField(TEXT("result"), NestedResult) && NestedResult)
		{
			const TArray<TSharedPtr<FJsonValue>>* Machines = nullptr;
			if ((*NestedResult)->TryGetArrayField(TEXT("state_machines"), Machines)
				|| (*NestedResult)->TryGetArrayField(TEXT("machines"), Machines)
				|| (*NestedResult)->TryGetArrayField(TEXT("stateMachines"), Machines))
			{
				if (Machines)
				{
					for (const TSharedPtr<FJsonValue>& V : *Machines)
					{
						const TSharedPtr<FJsonObject>* MObj = nullptr;
						if (!V->TryGetObject(MObj) || !MObj)
						{
							continue;
						}
						FString MachineName;
						if (!(*MObj)->TryGetStringField(TEXT("name"), MachineName))
						{
							(*MObj)->TryGetStringField(TEXT("machine_name"), MachineName);
						}
						if (MachineName.IsEmpty())
						{
							continue;
						}
						TSharedPtr<FJsonObject> TrParams = MakeShared<FJsonObject>();
						TrParams->SetStringField(TEXT("asset_path"), AssetPath);
						TrParams->SetStringField(TEXT("machine_name"), MachineName);
						const FString TrRel = FString::Printf(TEXT("abp/%s/transitions_%s.json"), *Label, *MachineName);
						FAxonActionResult Tr = InvokeAndWrite(PluginName, TEXT("animation"), TEXT("get_transitions"), TrParams, TrRel);
						if (Tr.bSuccess)
						{
							Written.Add(MakeShared<FJsonValueString>(Tr.Result->GetStringField(TEXT("written"))));
						}
					}
				}
			}
		}
	}

	Out->SetArrayField(TEXT("written_files"), Written);
	Out->SetStringField(TEXT("label"), Label);
	Out->SetStringField(TEXT("asset_path"), AssetPath);
	return FAxonActionResult::Success(Out);
}

FAxonActionResult FAxonKnowledgeExtract::HandleExtractAnimGraphOverview(const FString& PluginName, const TSharedPtr<FJsonObject>& Params)
{
	if (!Params.IsValid())
	{
		return FAxonActionResult::Error(TEXT("Missing params"));
	}
	FString AssetPath;
	if (!Params->TryGetStringField(TEXT("asset_path"), AssetPath) || AssetPath.IsEmpty())
	{
		return FAxonActionResult::Error(TEXT("Missing required parameter: asset_path"));
	}
	FString Label;
	if (!Params->TryGetStringField(TEXT("label"), Label) || Label.IsEmpty())
	{
		Label = SanitizeAssetFileStem(AssetPath);
	}

	TSharedPtr<FJsonObject> Base = MakeShared<FJsonObject>();
	Base->SetStringField(TEXT("asset_path"), AssetPath);

	struct FJob { const TCHAR* Action; const TCHAR* File; };
	const FJob Jobs[] = {
		{TEXT("get_graphs"), TEXT("graphs.json")},
		{TEXT("get_nodes"), TEXT("nodes.json")},
		{TEXT("get_anim_graph_choosers"), TEXT("anim_graph_choosers.json")},
		{TEXT("get_anim_node_function_bindings"), TEXT("anim_node_function_bindings.json")},
		{TEXT("get_abp_info"), TEXT("abp_info.json")},
	};

	TArray<TSharedPtr<FJsonValue>> Written;
	TArray<TSharedPtr<FJsonValue>> Errors;
	for (const FJob& Job : Jobs)
	{
		const FString Rel = FString::Printf(TEXT("abp/%s/%s"), *Label, Job.File);
		FAxonActionResult R = InvokeAndWrite(PluginName, TEXT("animation"), Job.Action, Base, Rel);
		if (R.bSuccess)
		{
			Written.Add(MakeShared<FJsonValueString>(R.Result->GetStringField(TEXT("written"))));
		}
		else
		{
			TSharedPtr<FJsonObject> E = MakeShared<FJsonObject>();
			E->SetStringField(TEXT("action"), Job.Action);
			E->SetStringField(TEXT("error"), R.ErrorMessage);
			Errors.Add(MakeShared<FJsonValueObject>(E));
		}
	}

	TSharedPtr<FJsonObject> Out = MakeShared<FJsonObject>();
	Out->SetArrayField(TEXT("written_files"), Written);
	Out->SetArrayField(TEXT("errors"), Errors);
	Out->SetStringField(TEXT("label"), Label);
	Out->SetStringField(TEXT("asset_path"), AssetPath);
	if (Written.Num() == 0)
	{
		return FAxonActionResult::Error(TEXT("extract_anim_graph_overview wrote no files — is AxonAnimation loaded?"));
	}
	return FAxonActionResult::Success(Out);
}

FAxonActionResult FAxonKnowledgeExtract::HandleExtractChooser(const FString& PluginName, const TSharedPtr<FJsonObject>& Params)
{
	if (!Params.IsValid())
	{
		return FAxonActionResult::Error(TEXT("Missing params"));
	}
	FString AssetPath;
	if (!Params->TryGetStringField(TEXT("asset_path"), AssetPath) || AssetPath.IsEmpty())
	{
		return FAxonActionResult::Error(TEXT("Missing required parameter: asset_path"));
	}
	FString Path;
	if (!Params->TryGetStringField(TEXT("path"), Path) || Path.IsEmpty())
	{
		Path = FString::Printf(TEXT("choosers/%s.json"), *SanitizeAssetFileStem(AssetPath));
	}
	TSharedPtr<FJsonObject> Fwd = MakeShared<FJsonObject>();
	Fwd->SetStringField(TEXT("asset_path"), AssetPath);
	Fwd->SetBoolField(TEXT("include_cells"), true);
	Fwd->SetBoolField(TEXT("recursive"), true);
	return InvokeAndWrite(PluginName, TEXT("chooser"), TEXT("inspect_chooser"), Fwd, Path);
}

FAxonActionResult FAxonKnowledgeExtract::HandleExtractConfigDdcvars(const FString& PluginName, const TSharedPtr<FJsonObject>& Params)
{
	FString Query = TEXT("ddcvar");
	FString Path = TEXT("config/ddcvars.json");
	if (Params.IsValid())
	{
		Params->TryGetStringField(TEXT("query"), Query);
		Params->TryGetStringField(TEXT("path"), Path);
	}
	TSharedPtr<FJsonObject> Fwd = MakeShared<FJsonObject>();
	Fwd->SetStringField(TEXT("query"), Query);
	return InvokeAndWrite(PluginName, TEXT("config"), TEXT("search_config"), Fwd, Path);
}

FAxonActionResult FAxonKnowledgeExtract::HandleExtractBundle(const FString& PluginName, const TSharedPtr<FJsonObject>& Params)
{
	if (!Params.IsValid())
	{
		return FAxonActionResult::Error(TEXT("Missing params"));
	}
	const TArray<TSharedPtr<FJsonValue>>* Jobs = nullptr;
	if (!Params->TryGetArrayField(TEXT("jobs"), Jobs) || !Jobs || Jobs->Num() == 0)
	{
		return FAxonActionResult::Error(TEXT("Missing required parameter: jobs (non-empty array)"));
	}

	TArray<TSharedPtr<FJsonValue>> Written;
	TArray<TSharedPtr<FJsonValue>> Errors;
	for (const TSharedPtr<FJsonValue>& V : *Jobs)
	{
		const TSharedPtr<FJsonObject>* JobObj = nullptr;
		if (!V->TryGetObject(JobObj) || !JobObj)
		{
			continue;
		}
		FAxonActionResult R = HandleExtractInvoke(PluginName, *JobObj);
		if (R.bSuccess)
		{
			Written.Add(MakeShared<FJsonValueString>(R.Result->GetStringField(TEXT("written"))));
		}
		else
		{
			TSharedPtr<FJsonObject> E = MakeShared<FJsonObject>();
			E->SetStringField(TEXT("error"), R.ErrorMessage);
			FString Path;
			(*JobObj)->TryGetStringField(TEXT("path"), Path);
			E->SetStringField(TEXT("path"), Path);
			Errors.Add(MakeShared<FJsonValueObject>(E));
		}
	}

	// Update manifest
	TSharedPtr<FJsonObject> Manifest = MakeShared<FJsonObject>();
	Manifest->SetArrayField(TEXT("written_files"), Written);
	Manifest->SetNumberField(TEXT("count"), Written.Num());
	FString ManifestAbs;
	FString ManifestErr;
	WriteJsonToRaw(PluginName, TEXT("_manifest.json"), Manifest, ManifestAbs, ManifestErr);

	TSharedPtr<FJsonObject> Out = MakeShared<FJsonObject>();
	Out->SetArrayField(TEXT("written_files"), Written);
	Out->SetArrayField(TEXT("errors"), Errors);
	Out->SetNumberField(TEXT("ok_count"), Written.Num());
	Out->SetNumberField(TEXT("error_count"), Errors.Num());
	if (!ManifestAbs.IsEmpty())
	{
		Out->SetStringField(TEXT("manifest"), ManifestAbs);
	}
	return FAxonActionResult::Success(Out);
}
