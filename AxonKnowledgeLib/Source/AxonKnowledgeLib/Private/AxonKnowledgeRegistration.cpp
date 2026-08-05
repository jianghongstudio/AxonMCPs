#include "AxonKnowledgeRegistration.h"
#include "AxonKnowledgeCorpus.h"
#include "AxonKnowledgeRegistry.h"
#include "AxonKnowledgeExtract.h"
#include "AxonCoreModule.h"
#include "AxonToolRegistry.h"
#include "AxonParamSchema.h"

namespace AxonKnowledgeRegInternal
{
	static bool WantReload(const TSharedPtr<FJsonObject>& Params)
	{
		bool bReload = false;
		if (Params.IsValid())
		{
			Params->TryGetBoolField(TEXT("reload"), bReload);
		}
		return bReload;
	}

	static FAxonActionResult HandleRoute(const FString& PluginName, const TSharedPtr<FJsonObject>& Params)
	{
		FString Query;
		if (!Params.IsValid() || !Params->TryGetStringField(TEXT("query"), Query) || Query.IsEmpty())
		{
			return FAxonActionResult::Error(TEXT("Missing required parameter: query"));
		}
		FAxonKnowledgeCorpus::Get(PluginName).EnsureLoaded(WantReload(Params));
		return FAxonActionResult::Success(FAxonKnowledgeCorpus::Get(PluginName).RouteJson(Query));
	}

	static FAxonActionResult HandleSearch(const FString& PluginName, const TSharedPtr<FJsonObject>& Params)
	{
		FString Query;
		if (!Params.IsValid() || !Params->TryGetStringField(TEXT("query"), Query) || Query.IsEmpty())
		{
			return FAxonActionResult::Error(TEXT("Missing required parameter: query"));
		}
		int32 MaxResults = 10;
		if (Params->HasField(TEXT("max_results")))
		{
			MaxResults = static_cast<int32>(Params->GetNumberField(TEXT("max_results")));
		}
		FAxonKnowledgeCorpus::Get(PluginName).EnsureLoaded(WantReload(Params));
		return FAxonActionResult::Success(FAxonKnowledgeCorpus::Get(PluginName).SearchJson(Query, MaxResults));
	}

	static FAxonActionResult HandleRead(const FString& PluginName, const TSharedPtr<FJsonObject>& Params)
	{
		FString Path;
		if (!Params.IsValid())
		{
			return FAxonActionResult::Error(TEXT("Missing required parameter: path"));
		}
		if (!Params->TryGetStringField(TEXT("path"), Path) || Path.IsEmpty())
		{
			Params->TryGetStringField(TEXT("doc"), Path);
		}
		if (Path.IsEmpty())
		{
			Params->TryGetStringField(TEXT("topic"), Path);
		}
		if (Path.IsEmpty())
		{
			return FAxonActionResult::Error(TEXT("Missing required parameter: path"));
		}

		bool bIncludeBody = true;
		Params->TryGetBoolField(TEXT("include_body"), bIncludeBody);

		FAxonKnowledgeCorpus::Get(PluginName).EnsureLoaded(WantReload(Params));
		TSharedPtr<FJsonObject> Result = FAxonKnowledgeCorpus::Get(PluginName).ReadJson(Path, bIncludeBody);
		if (!Result->GetBoolField(TEXT("found")))
		{
			return FAxonActionResult::Error(
				FString::Printf(TEXT("Knowledge doc not found: %s"), *Path),
				-32004);
		}
		return FAxonActionResult::Success(Result);
	}

	static FAxonActionResult HandleListTopics(const FString& PluginName, const TSharedPtr<FJsonObject>& Params)
	{
		FAxonKnowledgeCorpus::Get(PluginName).EnsureLoaded(WantReload(Params));
		return FAxonActionResult::Success(FAxonKnowledgeCorpus::Get(PluginName).ListTopicsJson());
	}
}

void FAxonKnowledgeRegistration::RegisterQueryActions(const FString& Namespace, const FString& PluginName)
{
	FAxonToolRegistry& Registry = FAxonToolRegistry::Get();
	const FString Plugin = PluginName;

	Registry.RegisterAction(
		Namespace, TEXT("route"),
		TEXT("Route a task or symptom to the best Knowledge markdown docs (from 00-routing.md tables)."),
		FAxonActionHandler::CreateLambda([Plugin](const TSharedPtr<FJsonObject>& Params)
		{
			return AxonKnowledgeRegInternal::HandleRoute(Plugin, Params);
		}),
		FParamSchemaBuilder()
			.Required(TEXT("query"), TEXT("string"), TEXT("Task, symptom, or free-text question to route"))
			.Optional(TEXT("reload"), TEXT("boolean"), TEXT("Force reload Knowledge/*.md from disk"), TEXT("false"))
			.Build());

	Registry.RegisterAction(
		Namespace, TEXT("search"),
		TEXT("Full-text search across the packaged Knowledge corpus; returns ranked snippets with doc paths."),
		FAxonActionHandler::CreateLambda([Plugin](const TSharedPtr<FJsonObject>& Params)
		{
			return AxonKnowledgeRegInternal::HandleSearch(Plugin, Params);
		}),
		FParamSchemaBuilder()
			.Required(TEXT("query"), TEXT("string"), TEXT("Search text"))
			.Optional(TEXT("max_results"), TEXT("integer"), TEXT("Max hits to return (1-50)"), TEXT("10"))
			.Optional(TEXT("reload"), TEXT("boolean"), TEXT("Force reload Knowledge/*.md from disk"), TEXT("false"))
			.Build());

	Registry.RegisterAction(
		Namespace, TEXT("read"),
		TEXT("Read a Knowledge markdown document by relative path or topic id."),
		FAxonActionHandler::CreateLambda([Plugin](const TSharedPtr<FJsonObject>& Params)
		{
			return AxonKnowledgeRegInternal::HandleRead(Plugin, Params);
		}),
		FParamSchemaBuilder()
			.Required(TEXT("path"), TEXT("string"), TEXT("Relative Knowledge path or basename"), {TEXT("doc"), TEXT("topic")})
			.Optional(TEXT("include_body"), TEXT("boolean"), TEXT("Include full markdown body (default true)"), TEXT("true"))
			.Optional(TEXT("reload"), TEXT("boolean"), TEXT("Force reload Knowledge/*.md from disk"), TEXT("false"))
			.Build());

	Registry.RegisterAction(
		Namespace, TEXT("list_topics"),
		TEXT("List available Knowledge topics/documents with short descriptions."),
		FAxonActionHandler::CreateLambda([Plugin](const TSharedPtr<FJsonObject>& Params)
		{
			return AxonKnowledgeRegInternal::HandleListTopics(Plugin, Params);
		}),
		FParamSchemaBuilder()
			.Optional(TEXT("reload"), TEXT("boolean"), TEXT("Force reload Knowledge/*.md from disk"), TEXT("false"))
			.Build());
}

void FAxonKnowledgeRegistration::RegisterExtractActions(const FString& Namespace, const FString& PluginName)
{
	FAxonToolRegistry& Registry = FAxonToolRegistry::Get();
	const FString Plugin = PluginName;

	Registry.RegisterAction(
		Namespace, TEXT("extract_write"),
		TEXT("Write a JSON object into this KB plugin Knowledge/_raw/<path>."),
		FAxonActionHandler::CreateLambda([Plugin](const TSharedPtr<FJsonObject>& Params)
		{
			return FAxonKnowledgeExtract::HandleExtractWrite(Plugin, Params);
		}),
		FParamSchemaBuilder()
			.Required(TEXT("path"), TEXT("string"), TEXT("Relative path under Knowledge/_raw (e.g. abp/cmc/state_machines.json)"))
			.Required(TEXT("json"), TEXT("object"), TEXT("JSON object payload to write"))
			.Build());

	Registry.RegisterAction(
		Namespace, TEXT("extract_invoke"),
		TEXT("Execute another Axon action and write its result JSON under Knowledge/_raw/."),
		FAxonActionHandler::CreateLambda([Plugin](const TSharedPtr<FJsonObject>& Params)
		{
			return FAxonKnowledgeExtract::HandleExtractInvoke(Plugin, Params);
		}),
		FParamSchemaBuilder()
			.Required(TEXT("target_namespace"), TEXT("string"), TEXT("Namespace to invoke (e.g. animation)"))
			.Required(TEXT("target_action"), TEXT("string"), TEXT("Action name (e.g. get_state_machines)"))
			.Required(TEXT("path"), TEXT("string"), TEXT("Relative path under Knowledge/_raw for the result"))
			.Optional(TEXT("params"), TEXT("object"), TEXT("Params forwarded to the target action"))
			.Build());

	Registry.RegisterAction(
		Namespace, TEXT("extract_state_machines"),
		TEXT("Dump ABP state machines (+ optional transitions) into Knowledge/_raw/abp/..."),
		FAxonActionHandler::CreateLambda([Plugin](const TSharedPtr<FJsonObject>& Params)
		{
			return FAxonKnowledgeExtract::HandleExtractStateMachines(Plugin, Params);
		}),
		FParamSchemaBuilder()
			.Required(TEXT("asset_path"), TEXT("string"), TEXT("Animation Blueprint asset path"))
			.Optional(TEXT("label"), TEXT("string"), TEXT("Subfolder under _raw/abp (default: stem of asset)"))
			.Optional(TEXT("include_transitions"), TEXT("boolean"), TEXT("Also dump get_transitions per machine"), TEXT("true"))
			.Build());

	Registry.RegisterAction(
		Namespace, TEXT("extract_anim_graph_overview"),
		TEXT("Dump ABP graphs/nodes/choosers/bindings overview into Knowledge/_raw/abp/..."),
		FAxonActionHandler::CreateLambda([Plugin](const TSharedPtr<FJsonObject>& Params)
		{
			return FAxonKnowledgeExtract::HandleExtractAnimGraphOverview(Plugin, Params);
		}),
		FParamSchemaBuilder()
			.Required(TEXT("asset_path"), TEXT("string"), TEXT("Animation Blueprint asset path"))
			.Optional(TEXT("label"), TEXT("string"), TEXT("Subfolder under _raw/abp"))
			.Build());

	Registry.RegisterAction(
		Namespace, TEXT("extract_chooser"),
		TEXT("Dump chooser_query inspect_chooser into Knowledge/_raw/choosers/..."),
		FAxonActionHandler::CreateLambda([Plugin](const TSharedPtr<FJsonObject>& Params)
		{
			return FAxonKnowledgeExtract::HandleExtractChooser(Plugin, Params);
		}),
		FParamSchemaBuilder()
			.Required(TEXT("asset_path"), TEXT("string"), TEXT("Chooser asset path"))
			.Optional(TEXT("path"), TEXT("string"), TEXT("Override relative _raw path"))
			.Build());

	Registry.RegisterAction(
		Namespace, TEXT("extract_config_ddcvars"),
		TEXT("Dump config search for ddcvar-like settings into Knowledge/_raw/config/ddcvars.json"),
		FAxonActionHandler::CreateLambda([Plugin](const TSharedPtr<FJsonObject>& Params)
		{
			return FAxonKnowledgeExtract::HandleExtractConfigDdcvars(Plugin, Params);
		}),
		FParamSchemaBuilder()
			.Optional(TEXT("query"), TEXT("string"), TEXT("config search query"), TEXT("ddcvar"))
			.Optional(TEXT("path"), TEXT("string"), TEXT("Relative _raw path"), TEXT("config/ddcvars.json"))
			.Build());

	Registry.RegisterAction(
		Namespace, TEXT("extract_bundle"),
		TEXT("Run a list of extract_invoke jobs and return a written-files manifest."),
		FAxonActionHandler::CreateLambda([Plugin](const TSharedPtr<FJsonObject>& Params)
		{
			return FAxonKnowledgeExtract::HandleExtractBundle(Plugin, Params);
		}),
		FParamSchemaBuilder()
			.Required(TEXT("jobs"), TEXT("array"), TEXT("Array of {target_namespace,target_action,path,params?}"))
			.Build());
}

void FAxonKnowledgeRegistration::RegisterAll(const FString& Namespace, const FString& PluginName, const FOptions& Options)
{
	RegisterQueryActions(Namespace, PluginName);
	if (Options.bIncludeExtract)
	{
		RegisterExtractActions(Namespace, PluginName);
	}
	FAxonKnowledgeRegistry::Register(PluginName, Namespace);
}

void FAxonKnowledgeRegistration::UnregisterAll(const FString& Namespace, const FString& PluginName)
{
	if (FAxonCoreModule::IsAvailable())
	{
		FAxonToolRegistry::Get().UnregisterNamespace(Namespace);
	}
	FAxonKnowledgeRegistry::Unregister(PluginName);
}
