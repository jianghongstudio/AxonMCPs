#include "AxonBlueprintModule.h"
#include "AxonBlueprintActions.h"
#include "AxonBlueprintVariableActions.h"
#include "AxonBlueprintContractActions.h"
#include "AxonBlueprintComponentActions.h"
#include "AxonBlueprintGraphActions.h"
#include "AxonBlueprintNodeActions.h"
#include "AxonBlueprintCompileActions.h"
#include "AxonBlueprintCDOActions.h"
#include "AxonBlueprintStructActions.h"
#include "AxonBlueprintDataTableActions.h"
#include "AxonBlueprintCurveTableActions.h"
#include "AxonBlueprintStringTableActions.h"
#include "AxonBlueprintBuildActions.h"
#include "AxonBlueprintDiffActions.h"
#include "AxonBlueprintTemplateActions.h"
#include "AxonBlueprintGraphExportActions.h"
#include "AxonBlueprintLayoutActions.h"
#include "AxonBlueprintSpawnActions.h"
#include "AxonMotionMatchingScaffoldActions.h"
#include "AxonBlueprintBulkFillAdapter.h"
#include "AxonToolRegistry.h"
#include "AxonJsonUtils.h"

#define LOCTEXT_NAMESPACE "FAxonBlueprintModule"

void FAxonBlueprintModule::StartupModule()
{
	FAxonToolRegistry& Registry = FAxonToolRegistry::Get();
	FAxonBlueprintActions::RegisterActions();
	FAxonBlueprintVariableActions::RegisterActions(Registry);
	FAxonBlueprintContractActions::RegisterActions(Registry);
	FAxonBlueprintComponentActions::RegisterActions(Registry);
	FAxonBlueprintGraphActions::RegisterActions(Registry);
	FAxonBlueprintNodeActions::RegisterActions(Registry);
	FAxonBlueprintCompileActions::RegisterActions(Registry);
	FAxonBlueprintCDOActions::RegisterActions(Registry);
	FAxonBlueprintStructActions::RegisterActions(Registry);
	FAxonBlueprintDataTableActions::RegisterActions(Registry);
	FAxonBlueprintCurveTableActions::RegisterActions(Registry);
	FAxonBlueprintStringTableActions::RegisterActions(Registry);
	FAxonBlueprintBuildActions::RegisterActions(Registry);
	FAxonBlueprintDiffActions::RegisterActions(Registry);
	FAxonBlueprintTemplateActions::RegisterActions(Registry);
	FAxonBlueprintGraphExportActions::RegisterActions(Registry);
	FAxonBlueprintLayoutActions::RegisterActions(Registry);
	FAxonBlueprintSpawnActions::RegisterActions(Registry);

	// Sprint 5 — Motion Matching Action Pack (Pillar D): character/actor BP scaffolding.
	FAxonMotionMatchingScaffoldActions::RegisterActions(Registry);

	// Phase 1 bulk_fill / describe pilot adapter. Self-registers with
	// FAxonBulkFillRegistry under namespace "blueprint"; routed-to by the
	// central bulk_fill.apply / describe.schema dispatchers (Phase 0).
	FAxonBlueprintBulkFillAdapter::Register();

	UE_LOG(LogAxon, Log, TEXT("Axon — Blueprint module loaded (110 actions + bulk_fill/describe adapter)"));
}

void FAxonBlueprintModule::ShutdownModule()
{
	FAxonBlueprintBulkFillAdapter::Unregister();
	FAxonToolRegistry::Get().UnregisterNamespace(TEXT("blueprint"));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FAxonBlueprintModule, AxonBlueprint)
