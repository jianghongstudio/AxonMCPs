#include "AxonAnimationModule.h"
#include "AxonJsonUtils.h"
#include "AxonAnimationActions.h"
#include "AxonAnimationRuntimeActions.h"
#include "AxonPoseSearchActions.h"
#include "AxonMirrorTableActions.h"
#include "AxonControlRigWriteActions.h"
#include "AxonAbpWriteActions.h"
#include "AxonAnimLayoutActions.h"
#include "AxonAnimationBulkFillAdapter.h"
#include "AxonChooserActions.h"
#include "AxonChooserAuthoringActions.h"
#include "AxonAbpGraphSurgeryActions.h"
#include "AxonRetargetSettingsActions.h"
#include "AxonSkeletonRetargetActions.h"
#include "AxonLocomotionAuthoringActions.h"
#include "AxonToolRegistry.h"

#define LOCTEXT_NAMESPACE "FAxonAnimationModule"

void FAxonAnimationModule::StartupModule()
{
	FAxonAnimationActions::RegisterActions(FAxonToolRegistry::Get());
	FAxonAnimationRuntimeActions::RegisterActions(FAxonToolRegistry::Get());
	FAxonPoseSearchActions::RegisterActions(FAxonToolRegistry::Get());
	FAxonMirrorTableActions::RegisterActions(FAxonToolRegistry::Get());
	FAxonControlRigWriteActions::RegisterActions(FAxonToolRegistry::Get());
	FAxonAbpWriteActions::RegisterActions(FAxonToolRegistry::Get());
	FAxonAnimLayoutActions::RegisterActions(FAxonToolRegistry::Get());
	FAxonChooserActions::RegisterActions(FAxonToolRegistry::Get());
	FAxonChooserAuthoringActions::RegisterActions(FAxonToolRegistry::Get());
	FAxonAbpGraphSurgeryActions::RegisterActions(FAxonToolRegistry::Get());
	FAxonRetargetSettingsActions::RegisterActions(FAxonToolRegistry::Get());
	FAxonSkeletonRetargetActions::RegisterActions(FAxonToolRegistry::Get());
	FAxonLocomotionAuthoringActions::RegisterActions(FAxonToolRegistry::Get());

	// Phase 5 Step 6 (MCP Ergonomics, 2026-05-11) — register the animation adapter.
	// PoseSearchDatabase fill_kind replaces the 40+ add_database_animation
	// round-trips per locomotion set (design B.3 pain point).
	FAxonAnimationBulkFillAdapter::Register();

	UE_LOG(LogAxon, Verbose, TEXT("Axon - Animation module loaded"));
}

void FAxonAnimationModule::ShutdownModule()
{
	FAxonAnimationBulkFillAdapter::Unregister();
	FAxonToolRegistry::Get().UnregisterNamespace(TEXT("animation"));
	FAxonToolRegistry::Get().UnregisterNamespace(TEXT("chooser"));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FAxonAnimationModule, AxonAnimation)
