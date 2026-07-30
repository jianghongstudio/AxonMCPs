#include "AxonCoreTools.h"
#include "AxonGuideTool.h"
#include "AxonCoreModule.h"
#include "AxonJsonUtils.h"
#include "AxonHttpServer.h"
#include "AxonSettings.h"
#include "AxonExtensionResearch.h"
#include "AxonExtensionScaffolder.h"
#include "Misc/App.h"

static FString AxonTerseOneLineDescription(const FString& Full)
{
	const int32 HardCap = 150;
	const int32 MinSentence = 25;
	const int32 Len = Full.Len();

	int32 SentenceEnd = MAX_int32;
	for (int32 Index = MinSentence; Index < Len; ++Index)
	{
		const TCHAR Ch = Full[Index];
		if (Ch == TEXT('.') || Ch == TEXT('!') || Ch == TEXT('?'))
		{
			const bool bFollowedBySpaceOrEnd = (Index + 1 >= Len) || FChar::IsWhitespace(Full[Index + 1]);
			if (bFollowedBySpaceOrEnd)
			{
				SentenceEnd = Index + 1;
				break;
			}
		}
	}

	int32 Cut = FMath::Min(SentenceEnd, HardCap);
	if (Cut >= Len)
	{
		return Full;
	}

	if (Cut == HardCap && !FChar::IsWhitespace(Full[Cut]))
	{
		int32 WordBoundary = Cut;
		while (WordBoundary > 0 && !FChar::IsWhitespace(Full[WordBoundary - 1]))
		{
			--WordBoundary;
		}
		if (WordBoundary > 0)
		{
			Cut = WordBoundary;
		}
	}

	FString Trimmed = Full.Left(Cut);
	int32 Tail = Trimmed.Len();
	while (Tail > 0)
	{
		const TCHAR Ch = Trimmed[Tail - 1];
		if (FChar::IsWhitespace(Ch) || Ch == TEXT('.') || Ch == TEXT('!') || Ch == TEXT('?'))
		{
			--Tail;
		}
		else
		{
			break;
		}
	}
	Trimmed.LeftInline(Tail);
	Trimmed += TEXT("...");
	return Trimmed;
}

void FAxonCoreTools::RegisterAll()
{
	FAxonToolRegistry& Registry = FAxonToolRegistry::Get();

	{
		TSharedPtr<FJsonObject> Schema = MakeShared<FJsonObject>();

		TSharedPtr<FJsonObject> NsProp = MakeShared<FJsonObject>();
		NsProp->SetStringField(TEXT("type"), TEXT("string"));
		NsProp->SetStringField(TEXT("description"), TEXT("Optional: filter to a specific namespace"));
		Schema->SetObjectField(TEXT("namespace"), NsProp);

		TSharedPtr<FJsonObject> CatProp = MakeShared<FJsonObject>();
		CatProp->SetStringField(TEXT("type"), TEXT("string"));
		CatProp->SetStringField(TEXT("description"), TEXT("Optional: filter actions within the namespace by category"));
		Schema->SetObjectField(TEXT("category"), CatProp);

		TSharedPtr<FJsonObject> DetailProp = MakeShared<FJsonObject>();
		DetailProp->SetStringField(TEXT("type"), TEXT("boolean"));
		DetailProp->SetStringField(TEXT("description"), TEXT("Optional: inline the full param schema for every action (default false = terse). 'verbose' is an accepted alias. Prefer describe_query action_schema for a single action's schema."));
		Schema->SetObjectField(TEXT("detail"), DetailProp);

		TSharedPtr<FJsonObject> VerboseProp = MakeShared<FJsonObject>();
		VerboseProp->SetStringField(TEXT("type"), TEXT("boolean"));
		VerboseProp->SetStringField(TEXT("description"), TEXT("Alias for detail; inline full param schemas. Prefer detail."));
		Schema->SetObjectField(TEXT("verbose"), VerboseProp);

		TSharedPtr<FJsonObject> FilterProp = MakeShared<FJsonObject>();
		FilterProp->SetStringField(TEXT("type"), TEXT("string"));
		FilterProp->SetStringField(TEXT("description"), TEXT("Optional: case-insensitive substring matched against each action's name or description."));
		Schema->SetObjectField(TEXT("filter"), FilterProp);

		TSharedPtr<FJsonObject> OffsetProp = MakeShared<FJsonObject>();
		OffsetProp->SetStringField(TEXT("type"), TEXT("integer"));
		OffsetProp->SetStringField(TEXT("description"), TEXT("Optional: pagination start index (default 0). Only meaningful when limit > 0."));
		Schema->SetObjectField(TEXT("offset"), OffsetProp);

		TSharedPtr<FJsonObject> LimitProp = MakeShared<FJsonObject>();
		LimitProp->SetStringField(TEXT("type"), TEXT("integer"));
		LimitProp->SetStringField(TEXT("description"), TEXT("Optional: max actions to return (default 0 = ALL)."));
		Schema->SetObjectField(TEXT("limit"), LimitProp);

		Registry.RegisterAction(
			TEXT("axon"), TEXT("discover"),
			TEXT("List available tool namespaces and their actions. Pass namespace (and optional category) to filter. Per-namespace output is terse by default; pass detail=true to inline param schemas, or use describe_query action_schema for one action."),
			FAxonActionHandler::CreateStatic(&FAxonCoreTools::HandleDiscover),
			Schema
		);
		Registry.SetActionAnnotations(TEXT("axon"), TEXT("discover"),
			/*bReadOnly=*/true, /*bDestructive=*/false, /*bIdempotent=*/true,
			TEXT("Discover Axon actions"));
	}

	{
		Registry.RegisterAction(
			TEXT("axon"), TEXT("status"),
			TEXT("Get Axon server health: version, uptime, port, registered action count, namespace count."),
			FAxonActionHandler::CreateStatic(&FAxonCoreTools::HandleStatus)
		);
		Registry.SetActionAnnotations(TEXT("axon"), TEXT("status"),
			/*bReadOnly=*/true, /*bDestructive=*/false, /*bIdempotent=*/true,
			TEXT("Axon server status"));
	}

	{
		TSharedPtr<FJsonObject> Schema = MakeShared<FJsonObject>();
		TSharedPtr<FJsonObject> Target = MakeShared<FJsonObject>();
		Target->SetStringField(TEXT("type"), TEXT("string"));
		Target->SetStringField(TEXT("description"), TEXT("Installed plugin name or filesystem path to research"));
		Target->SetBoolField(TEXT("required"), true);
		Schema->SetObjectField(TEXT("target_plugin"), Target);
		TSharedPtr<FJsonObject> Extra = MakeShared<FJsonObject>();
		Extra->SetStringField(TEXT("type"), TEXT("array"));
		Extra->SetStringField(TEXT("description"), TEXT("Optional extra directories to scan for headers/docs"));
		Schema->SetObjectField(TEXT("extra_paths"), Extra);

		Registry.RegisterAction(
			TEXT("axon"), TEXT("research_extension_target"),
			TEXT("Domain-agnostic scan of a target plugin (modules, public headers, docs). Does NOT recommend actions. Use AFTER the user agrees they want recommendations, BEFORE proposing an Action list."),
			FAxonActionHandler::CreateStatic(&FAxonCoreTools::HandleResearchExtensionTarget),
			Schema);
		Registry.SetActionAnnotations(TEXT("axon"), TEXT("research_extension_target"),
			true, false, true, TEXT("Research extension target"));
	}

	{
		TSharedPtr<FJsonObject> Schema = MakeShared<FJsonObject>();
		auto AddStr = [&Schema](const TCHAR* Name, const TCHAR* Desc, bool bRequired)
		{
			TSharedPtr<FJsonObject> P = MakeShared<FJsonObject>();
			P->SetStringField(TEXT("type"), TEXT("string"));
			P->SetStringField(TEXT("description"), Desc);
			P->SetBoolField(TEXT("required"), bRequired);
			Schema->SetObjectField(Name, P);
		};
		AddStr(TEXT("plugin_name"), TEXT("New sibling plugin name (e.g. AxonFoo)"), true);
		AddStr(TEXT("namespace"), TEXT("MCP namespace snake_case (e.g. foo). Derived from plugin_name if omitted."), false);
		AddStr(TEXT("intent"), TEXT("Optional blurb written into generated comments"), false);
		AddStr(TEXT("recipe_id"), TEXT("Optional external Templates/ExtensionRecipes/<id>.json"), false);
		AddStr(TEXT("research_ref"), TEXT("Optional research summary/hash for traceability"), false);

		TSharedPtr<FJsonObject> Deps = MakeShared<FJsonObject>();
		Deps->SetStringField(TEXT("type"), TEXT("array"));
		Deps->SetStringField(TEXT("description"), TEXT("Optional uplugin hard dependencies (besides Axon)"));
		Schema->SetObjectField(TEXT("depend_plugins"), Deps);

		TSharedPtr<FJsonObject> Mods = MakeShared<FJsonObject>();
		Mods->SetStringField(TEXT("type"), TEXT("array"));
		Mods->SetStringField(TEXT("description"), TEXT("Optional extra Build.cs module dependencies"));
		Schema->SetObjectField(TEXT("build_modules"), Mods);

		TSharedPtr<FJsonObject> Acts = MakeShared<FJsonObject>();
		Acts->SetStringField(TEXT("type"), TEXT("array"));
		Acts->SetStringField(TEXT("description"), TEXT("Confirmed actions [{name, description}]. Required unless skeleton_only=true. NEVER auto-invent without user consent + research."));
		Schema->SetObjectField(TEXT("actions"), Acts);

		TSharedPtr<FJsonObject> Skel = MakeShared<FJsonObject>();
		Skel->SetStringField(TEXT("type"), TEXT("boolean"));
		Skel->SetStringField(TEXT("description"), TEXT("If true, allow empty actions (compilable empty shell)"));
		Skel->SetStringField(TEXT("default"), TEXT("false"));
		Schema->SetObjectField(TEXT("skeleton_only"), Skel);

		TSharedPtr<FJsonObject> Dry = MakeShared<FJsonObject>();
		Dry->SetStringField(TEXT("type"), TEXT("boolean"));
		Dry->SetStringField(TEXT("description"), TEXT("If true, return plan only — do not write files"));
		Dry->SetStringField(TEXT("default"), TEXT("false"));
		Schema->SetObjectField(TEXT("dry_run"), Dry);

		Registry.RegisterAction(
			TEXT("axon"), TEXT("create_extension"),
			TEXT("Scaffold a generic Axon sibling plugin. Workflow gate: ask user if they want Action recommendations; if yes research first and confirm the list; then call with actions. Empty actions requires skeleton_only=true."),
			FAxonActionHandler::CreateStatic(&FAxonCoreTools::HandleCreateExtension),
			Schema);
	}

	{
		Registry.RegisterAction(
			TEXT("axon"), TEXT("list_extension_recipes"),
			TEXT("List optional external recipe JSON files under Templates/ExtensionRecipes (may be empty). Core ships no business-domain recipes."),
			FAxonActionHandler::CreateStatic(&FAxonCoreTools::HandleListExtensionRecipes));
		Registry.SetActionAnnotations(TEXT("axon"), TEXT("list_extension_recipes"),
			true, false, true, TEXT("List extension recipes"));
	}

	FAxonGuideTool::RegisterAll();
}

FAxonActionResult FAxonCoreTools::HandleDiscover(const TSharedPtr<FJsonObject>& Params)
{
	FAxonToolRegistry& Registry = FAxonToolRegistry::Get();

	FString FilterNamespace;
	FString FilterCategory;
	if (Params.IsValid())
	{
		Params->TryGetStringField(TEXT("namespace"), FilterNamespace);
		Params->TryGetStringField(TEXT("category"), FilterCategory);
	}

	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	TArray<FString> Namespaces = Registry.GetNamespaces();

	if (!FilterNamespace.IsEmpty())
	{
		TArray<FAxonActionInfo> Actions = Registry.GetActions(FilterNamespace);
		if (Actions.Num() == 0)
		{
			return FAxonActionResult::Error(
				FString::Printf(TEXT("Unknown namespace: %s — call axon_discover() to enumerate."), *FilterNamespace),
				FAxonJsonUtils::ErrInvalidParams
			);
		}

		if (!FilterCategory.IsEmpty())
		{
			Actions = Actions.FilterByPredicate([&FilterCategory](const FAxonActionInfo& Info)
			{
				return Info.Category.Equals(FilterCategory, ESearchCase::IgnoreCase);
			});
		}

		bool bDetail = false;
		Params->TryGetBoolField(TEXT("detail"), bDetail);
		if (!bDetail)
		{
			Params->TryGetBoolField(TEXT("verbose"), bDetail);
		}

		FString Filter;
		if (Params->TryGetStringField(TEXT("filter"), Filter) && !Filter.IsEmpty())
		{
			Actions = Actions.FilterByPredicate([&Filter](const FAxonActionInfo& Info)
			{
				return Info.Action.Contains(Filter, ESearchCase::IgnoreCase)
					|| Info.Description.Contains(Filter, ESearchCase::IgnoreCase);
			});
		}

		const int32 TotalCount = Actions.Num();
		int32 Offset = 0;
		int32 Limit = 0;
		Params->TryGetNumberField(TEXT("offset"), Offset);
		Params->TryGetNumberField(TEXT("limit"), Limit);

		int32 SliceStart = 0;
		int32 SliceEnd = TotalCount;
		if (Limit > 0)
		{
			SliceStart = FMath::Clamp(Offset, 0, TotalCount);
			SliceEnd = FMath::Clamp(SliceStart + Limit, SliceStart, TotalCount);
		}

		Result->SetStringField(TEXT("namespace"), FilterNamespace);
		if (!FilterCategory.IsEmpty())
		{
			Result->SetStringField(TEXT("category"), FilterCategory);
		}

		TArray<TSharedPtr<FJsonValue>> ActionArray;
		for (int32 Index = SliceStart; Index < SliceEnd; ++Index)
		{
			const FAxonActionInfo& ActionInfo = Actions[Index];
			TSharedPtr<FJsonObject> ActionObj = MakeShared<FJsonObject>();
			ActionObj->SetStringField(TEXT("action"), ActionInfo.Action);
			ActionObj->SetStringField(TEXT("description"),
				bDetail ? ActionInfo.Description : AxonTerseOneLineDescription(ActionInfo.Description));
			if (!ActionInfo.Category.IsEmpty())
			{
				ActionObj->SetStringField(TEXT("category"), ActionInfo.Category);
			}
			if (bDetail && ActionInfo.ParamSchema.IsValid())
			{
				ActionObj->SetObjectField(TEXT("params"), ActionInfo.ParamSchema);
			}
			ActionArray.Add(MakeShared<FJsonValueObject>(ActionObj));
		}
		Result->SetArrayField(TEXT("actions"), ActionArray);
		Result->SetNumberField(TEXT("total"), TotalCount);
		if (Limit > 0 && SliceEnd < TotalCount)
		{
			Result->SetNumberField(TEXT("next_offset"), SliceStart + Limit);
		}
		if (!bDetail)
		{
			Result->SetStringField(TEXT("schema_hint"),
				FString::Printf(TEXT("Param schemas omitted. Call describe_query(action_schema, target_namespace=\"%s\", target_action=\"<name>\") for one action's full schema, or pass detail=true to inline all."),
					*FilterNamespace));
		}
	}
	else
	{
		TArray<TSharedPtr<FJsonValue>> NsArray;
		for (const FString& Ns : Namespaces)
		{
			TArray<FAxonActionInfo> Actions = Registry.GetActions(Ns);
			TSharedPtr<FJsonObject> NsObj = MakeShared<FJsonObject>();
			NsObj->SetStringField(TEXT("namespace"), Ns);
			NsObj->SetNumberField(TEXT("action_count"), Actions.Num());

			TArray<TSharedPtr<FJsonValue>> ActionNames;
			for (const FAxonActionInfo& ActionInfo : Actions)
			{
				ActionNames.Add(MakeShared<FJsonValueString>(ActionInfo.Action));
			}
			NsObj->SetArrayField(TEXT("actions"), ActionNames);
			NsArray.Add(MakeShared<FJsonValueObject>(NsObj));
		}

		Result->SetArrayField(TEXT("namespaces"), NsArray);
		Result->SetNumberField(TEXT("total_actions"), Registry.GetActionCount());
		Result->SetStringField(TEXT("guide_hint"), TEXT("Call axon_guide() for workflow recipes and extension guidance. Section-keyed to bound context cost."));
	}

	return FAxonActionResult::Success(Result);
}

FAxonActionResult FAxonCoreTools::HandleStatus(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetStringField(TEXT("version"), AXON_VERSION);

	FAxonHttpServer* Server = FAxonCoreModule::Get().GetHttpServer();
	Result->SetBoolField(TEXT("server_running"), Server != nullptr && Server->IsRunning());
	Result->SetNumberField(TEXT("server_port"), Server ? Server->GetPort() : 0);

	FAxonToolRegistry& Registry = FAxonToolRegistry::Get();
	Result->SetNumberField(TEXT("total_actions"), Registry.GetActionCount());
	Result->SetNumberField(TEXT("namespaces"), Registry.GetNamespaces().Num());
	Result->SetStringField(TEXT("engine_version"), FApp::GetBuildVersion());
	Result->SetStringField(TEXT("project_name"), FApp::GetProjectName());

	return FAxonActionResult::Success(Result);
}

FAxonActionResult FAxonCoreTools::HandleResearchExtensionTarget(const TSharedPtr<FJsonObject>& Params)
{
	if (!Params.IsValid())
	{
		return FAxonActionResult::Error(TEXT("research_extension_target requires params"), FAxonJsonUtils::ErrInvalidParams);
	}

	FString TargetPlugin;
	Params->TryGetStringField(TEXT("target_plugin"), TargetPlugin);
	TArray<FString> ExtraPaths;
	const TArray<TSharedPtr<FJsonValue>>* ExtraArr = nullptr;
	if (Params->TryGetArrayField(TEXT("extra_paths"), ExtraArr) && ExtraArr)
	{
		for (const TSharedPtr<FJsonValue>& V : *ExtraArr)
		{
			ExtraPaths.Add(V->AsString());
		}
	}

	FString Err;
	TSharedPtr<FJsonObject> Findings = FAxonExtensionResearch::Research(TargetPlugin, ExtraPaths, Err);
	if (!Findings.IsValid())
	{
		return FAxonActionResult::Error(Err, FAxonJsonUtils::ErrInvalidParams);
	}
	return FAxonActionResult::Success(Findings);
}

FAxonActionResult FAxonCoreTools::HandleCreateExtension(const TSharedPtr<FJsonObject>& Params)
{
	if (!Params.IsValid())
	{
		return FAxonActionResult::Error(TEXT("create_extension requires params"), FAxonJsonUtils::ErrInvalidParams);
	}

	FAxonScaffoldRequest Req;
	Params->TryGetStringField(TEXT("plugin_name"), Req.PluginName);
	Params->TryGetStringField(TEXT("namespace"), Req.Namespace);
	Params->TryGetStringField(TEXT("intent"), Req.Intent);
	Params->TryGetStringField(TEXT("research_ref"), Req.ResearchRef);
	Params->TryGetBoolField(TEXT("skeleton_only"), Req.bSkeletonOnly);
	Params->TryGetBoolField(TEXT("dry_run"), Req.bDryRun);

	const TArray<TSharedPtr<FJsonValue>>* Deps = nullptr;
	if (Params->TryGetArrayField(TEXT("depend_plugins"), Deps) && Deps)
	{
		for (const TSharedPtr<FJsonValue>& V : *Deps)
		{
			Req.DependPlugins.Add(V->AsString());
		}
	}
	const TArray<TSharedPtr<FJsonValue>>* Mods = nullptr;
	if (Params->TryGetArrayField(TEXT("build_modules"), Mods) && Mods)
	{
		for (const TSharedPtr<FJsonValue>& V : *Mods)
		{
			Req.BuildModules.Add(V->AsString());
		}
	}
	const TArray<TSharedPtr<FJsonValue>>* Acts = nullptr;
	if (Params->TryGetArrayField(TEXT("actions"), Acts) && Acts)
	{
		for (const TSharedPtr<FJsonValue>& V : *Acts)
		{
			const TSharedPtr<FJsonObject>* AObj = nullptr;
			if (!V->TryGetObject(AObj) || !AObj)
			{
				continue;
			}
			FAxonScaffoldActionSpec Spec;
			(*AObj)->TryGetStringField(TEXT("name"), Spec.Name);
			(*AObj)->TryGetStringField(TEXT("description"), Spec.Description);
			if (Spec.Description.IsEmpty())
			{
				Spec.Description = FString::Printf(TEXT("TODO: implement %s"), *Spec.Name);
			}
			const TSharedPtr<FJsonObject>* ParamsSchema = nullptr;
			if ((*AObj)->TryGetObjectField(TEXT("params"), ParamsSchema) && ParamsSchema)
			{
				Spec.ParamsSchema = *ParamsSchema;
			}
			Req.Actions.Add(Spec);
		}
	}

	FString RecipeId;
	if (Params->TryGetStringField(TEXT("recipe_id"), RecipeId) && !RecipeId.IsEmpty())
	{
		FString RecipeErr;
		if (!FAxonExtensionScaffolder::LoadRecipe(RecipeId, Req, RecipeErr))
		{
			return FAxonActionResult::Error(RecipeErr, FAxonJsonUtils::ErrInvalidParams);
		}
	}

	if (Req.PluginName.IsEmpty())
	{
		return FAxonActionResult::Error(TEXT("plugin_name is required"), FAxonJsonUtils::ErrInvalidParams);
	}

	const FAxonScaffoldResult Scaffolded = FAxonExtensionScaffolder::Create(Req);
	if (!Scaffolded.bSuccess)
	{
		return FAxonActionResult::Error(Scaffolded.ErrorMessage, FAxonJsonUtils::ErrInvalidParams);
	}
	return FAxonActionResult::Success(Scaffolded.Payload);
}

FAxonActionResult FAxonCoreTools::HandleListExtensionRecipes(const TSharedPtr<FJsonObject>& Params)
{
	FString Err;
	TSharedPtr<FJsonObject> Result = FAxonExtensionScaffolder::ListRecipes(Err);
	if (!Result.IsValid())
	{
		return FAxonActionResult::Error(Err.IsEmpty() ? TEXT("list_extension_recipes failed") : Err);
	}
	return FAxonActionResult::Success(Result);
}
