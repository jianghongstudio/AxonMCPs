#include "AxonKnowledgeMetaActions.h"
#include "AxonKnowledgeScaffold.h"
#include "AxonKnowledgeRegistry.h"
#include "AxonParamSchema.h"
#include "Misc/App.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"

void FAxonKnowledgeMetaActions::RegisterAll()
{
	FAxonToolRegistry& Registry = FAxonToolRegistry::Get();

	Registry.RegisterAction(
		TEXT("knowledge"), TEXT("preview_kb_names"),
		TEXT("Derive Axon{Project}KB / {snake}_kb names from a project name (default: current project)."),
		FAxonActionHandler::CreateStatic(&FAxonKnowledgeMetaActions::HandlePreviewKbNames),
		FParamSchemaBuilder()
			.Optional(TEXT("project_name"), TEXT("string"), TEXT("Project name; default FApp::GetProjectName()"))
			.Build());
	Registry.SetActionAnnotations(TEXT("knowledge"), TEXT("preview_kb_names"),
		true, false, true, TEXT("Preview KB plugin names"));

	Registry.RegisterAction(
		TEXT("knowledge"), TEXT("list_kb_packs"),
		TEXT("List knowledge packs that registered via FAxonKnowledgeRegistration::RegisterAll (runtime index)."),
		FAxonActionHandler::CreateStatic(&FAxonKnowledgeMetaActions::HandleListKbPacks),
		FParamSchemaBuilder().Build());
	Registry.SetActionAnnotations(TEXT("knowledge"), TEXT("list_kb_packs"),
		true, false, true, TEXT("List registered KB packs"));

	Registry.RegisterAction(
		TEXT("knowledge"), TEXT("scaffold_kb_plugin"),
		TEXT("Scaffold a standalone AxonXxxKB sibling plugin (Knowledge/ + thin RegisterAll). Triggered by distill/蒸馏 current project. Prefer dry_run=true first."),
		FAxonActionHandler::CreateStatic(&FAxonKnowledgeMetaActions::HandleScaffoldKbPlugin),
		FParamSchemaBuilder()
			.Optional(TEXT("project_name"), TEXT("string"), TEXT("Project name; default current project"))
			.Optional(TEXT("plugin_name"), TEXT("string"), TEXT("Override plugin folder/module name"))
			.Optional(TEXT("namespace"), TEXT("string"), TEXT("Override MCP namespace"))
			.Optional(TEXT("dry_run"), TEXT("boolean"), TEXT("Plan only — do not write files"), TEXT("false"))
			.Optional(TEXT("force"), TEXT("boolean"), TEXT("Overwrite scaffolding files if plugin dir exists"), TEXT("false"))
			.Build());

	Registry.SetDispatcherAnnotations(TEXT("knowledge"),
		FAxonDispatcherAnnotations{false, false, false, TEXT("Axon knowledge pack scaffolding")});
}

FAxonActionResult FAxonKnowledgeMetaActions::HandleListKbPacks(const TSharedPtr<FJsonObject>& /*Params*/)
{
	TSharedPtr<FJsonObject> Out = MakeShared<FJsonObject>();
	TArray<TSharedPtr<FJsonValue>> Packs;
	for (const FAxonKnowledgePackInfo& Info : FAxonKnowledgeRegistry::GetRegistered())
	{
		TSharedPtr<FJsonObject> Row = MakeShared<FJsonObject>();
		Row->SetStringField(TEXT("plugin_name"), Info.PluginName);
		Row->SetStringField(TEXT("namespace"), Info.Namespace);
		Row->SetStringField(TEXT("mcp_tool"), Info.Namespace + TEXT("_query"));
		Packs.Add(MakeShared<FJsonValueObject>(Row));
	}
	Out->SetArrayField(TEXT("packs"), Packs);
	Out->SetNumberField(TEXT("count"), Packs.Num());
	return FAxonActionResult::Success(Out);
}

FAxonActionResult FAxonKnowledgeMetaActions::HandlePreviewKbNames(const TSharedPtr<FJsonObject>& Params)
{
	FString ProjectName;
	if (Params.IsValid())
	{
		Params->TryGetStringField(TEXT("project_name"), ProjectName);
	}
	if (ProjectName.IsEmpty())
	{
		ProjectName = FApp::GetProjectName();
	}
	const FAxonKbNamePlan Plan = FAxonKnowledgeScaffold::DeriveNames(ProjectName);
	TSharedPtr<FJsonObject> Out = MakeShared<FJsonObject>();
	Out->SetStringField(TEXT("project_name"), Plan.ProjectName);
	Out->SetStringField(TEXT("plugin_name"), Plan.PluginName);
	Out->SetStringField(TEXT("namespace"), Plan.Namespace);
	Out->SetStringField(TEXT("mcp_tool"), Plan.McpTool);
	Out->SetStringField(TEXT("plugin_path"), Plan.PluginPath);
	return FAxonActionResult::Success(Out);
}

FAxonActionResult FAxonKnowledgeMetaActions::HandleScaffoldKbPlugin(const TSharedPtr<FJsonObject>& Params)
{
	FAxonKbScaffoldRequest Req;
	if (Params.IsValid())
	{
		Params->TryGetStringField(TEXT("project_name"), Req.ProjectName);
		Params->TryGetStringField(TEXT("plugin_name"), Req.PluginName);
		Params->TryGetStringField(TEXT("namespace"), Req.Namespace);
		Params->TryGetBoolField(TEXT("dry_run"), Req.bDryRun);
		Params->TryGetBoolField(TEXT("force"), Req.bForce);
	}

	FAxonKbScaffoldResult Created = FAxonKnowledgeScaffold::Create(Req);
	if (!Created.bSuccess)
	{
		FAxonActionResult Err = FAxonActionResult::Error(
			Created.ErrorMessage.IsEmpty() ? TEXT("scaffold_kb_plugin failed") : Created.ErrorMessage);
		if (Created.Payload.IsValid())
		{
			Err.WithErrorData(Created.Payload);
		}
		return Err;
	}
	return FAxonActionResult::Success(Created.Payload);
}
