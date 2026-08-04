#include "AxonIndexModule.h"
#include "AxonIndexDatabase.h"
#include "AxonToolRegistry.h"
#include "AxonCoreModule.h"
#include "Actions/ProjectSearchAction.h"
#include "Actions/ProjectFindReferencesAction.h"
#include "Actions/ProjectFindByTypeAction.h"
#include "Actions/ProjectGetStatsAction.h"
#include "Actions/ProjectGetAssetDetailsAction.h"
#include "Actions/ProjectListGameplayTagsAction.h"
#include "Actions/ProjectSearchGameplayTagsAction.h"
#include "Actions/ProjectRefreshAssetsAction.h"
#include "Actions/ProjectGetSavedAssetStateAction.h"
#include "Actions/ProjectCleanupGeneratedAssetsAction.h"
#include "Actions/ProjectExportAssetTextAction.h"

#define LOCTEXT_NAMESPACE "FAxonIndexModule"

void FAxonIndexModule::StartupModule()
{
	UE_LOG(LogAxonIndex, Verbose, TEXT("Axon -- Index module loaded (11 actions, SQLite+FTS5)"));

	FAxonToolRegistry& Registry = FAxonToolRegistry::Get();

	Registry.RegisterAction(TEXT("project"), FProjectSearchAction::GetName(),
		FProjectSearchAction::GetDescription(),
		FAxonActionHandler::CreateStatic(&FProjectSearchAction::Execute),
		FProjectSearchAction::GetSchema());

	Registry.RegisterAction(TEXT("project"), FProjectFindReferencesAction::GetName(),
		FProjectFindReferencesAction::GetDescription(),
		FAxonActionHandler::CreateStatic(&FProjectFindReferencesAction::Execute),
		FProjectFindReferencesAction::GetSchema());

	Registry.RegisterAction(TEXT("project"), FProjectFindByTypeAction::GetName(),
		FProjectFindByTypeAction::GetDescription(),
		FAxonActionHandler::CreateStatic(&FProjectFindByTypeAction::Execute),
		FProjectFindByTypeAction::GetSchema());

	Registry.RegisterAction(TEXT("project"), FProjectGetStatsAction::GetName(),
		FProjectGetStatsAction::GetDescription(),
		FAxonActionHandler::CreateStatic(&FProjectGetStatsAction::Execute),
		FProjectGetStatsAction::GetSchema());

	Registry.RegisterAction(TEXT("project"), FProjectGetAssetDetailsAction::GetName(),
		FProjectGetAssetDetailsAction::GetDescription(),
		FAxonActionHandler::CreateStatic(&FProjectGetAssetDetailsAction::Execute),
		FProjectGetAssetDetailsAction::GetSchema());

	Registry.RegisterAction(TEXT("project"), FProjectListGameplayTagsAction::GetName(),
		FProjectListGameplayTagsAction::GetDescription(),
		FAxonActionHandler::CreateStatic(&FProjectListGameplayTagsAction::Execute),
		FProjectListGameplayTagsAction::GetSchema());

	Registry.RegisterAction(TEXT("project"), FProjectSearchGameplayTagsAction::GetName(),
		FProjectSearchGameplayTagsAction::GetDescription(),
		FAxonActionHandler::CreateStatic(&FProjectSearchGameplayTagsAction::Execute),
		FProjectSearchGameplayTagsAction::GetSchema());

	Registry.RegisterAction(TEXT("project"), FProjectRefreshAssetsAction::GetName(),
		FProjectRefreshAssetsAction::GetDescription(),
		FAxonActionHandler::CreateStatic(&FProjectRefreshAssetsAction::Execute),
		FProjectRefreshAssetsAction::GetSchema());

	Registry.RegisterAction(TEXT("project"), FProjectGetSavedAssetStateAction::GetName(),
		FProjectGetSavedAssetStateAction::GetDescription(),
		FAxonActionHandler::CreateStatic(&FProjectGetSavedAssetStateAction::Execute),
		FProjectGetSavedAssetStateAction::GetSchema());

	Registry.RegisterAction(TEXT("project"), FProjectCleanupGeneratedAssetsAction::GetName(),
		FProjectCleanupGeneratedAssetsAction::GetDescription(),
		FAxonActionHandler::CreateStatic(&FProjectCleanupGeneratedAssetsAction::Execute),
		FProjectCleanupGeneratedAssetsAction::GetSchema());

	Registry.RegisterAction(TEXT("project"), FProjectExportAssetTextAction::GetName(),
		FProjectExportAssetTextAction::GetDescription(),
		FAxonActionHandler::CreateStatic(&FProjectExportAssetTextAction::Execute),
		FProjectExportAssetTextAction::GetSchema());
}

void FAxonIndexModule::ShutdownModule()
{
	if (FAxonCoreModule::IsAvailable())
	{
		FAxonToolRegistry::Get().UnregisterNamespace(TEXT("project"));
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FAxonIndexModule, AxonIndex)
