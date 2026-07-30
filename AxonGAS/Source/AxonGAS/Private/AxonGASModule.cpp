#include "AxonGASModule.h"
#include "AxonToolRegistry.h"
#include "AxonGASAbilityActions.h"
#include "AxonGASAttributeActions.h"
#include "AxonGASEffectActions.h"
#include "AxonGASASCActions.h"
#include "AxonGASTagActions.h"
#include "AxonGASCueActions.h"
#include "AxonGASTargetActions.h"
#include "AxonGASInputActions.h"
#include "AxonGASInspectActions.h"
#include "AxonGASScaffoldActions.h"
#include "AxonGASUIBindingActions.h"
#include "AxonGASBulkFillAdapter.h"

DEFINE_LOG_CATEGORY(LogAxonGAS);

void FAxonGASModule::StartupModule()
{
	FAxonToolRegistry& Registry = FAxonToolRegistry::Get();
	FAxonGASAbilityActions::RegisterActions(Registry);
	FAxonGASAttributeActions::RegisterActions(Registry);
	FAxonGASEffectActions::RegisterActions(Registry);
	FAxonGASASCActions::RegisterActions(Registry);
	FAxonGASTagActions::RegisterActions(Registry);
	FAxonGASCueActions::RegisterActions(Registry);
	FAxonGASTargetActions::RegisterActions(Registry);
	FAxonGASInputActions::RegisterActions(Registry);
	FAxonGASInspectActions::RegisterActions(Registry);
	FAxonGASScaffoldActions::RegisterActions(Registry);
	FAxonGASUIBindingActions::RegisterActions(Registry);
	FAxonGASBulkFillAdapter::Register();

	const int32 ActionCount = Registry.GetActions(TEXT("gas")).Num();
	const TCHAR* GbaStatus =
#if WITH_GBA
		TEXT("available");
#else
		TEXT("not installed");
#endif
	UE_LOG(LogAxonGAS, Log, TEXT("AxonGAS: Loaded (%d actions, GBA=%s)"), ActionCount, GbaStatus);
}

void FAxonGASModule::ShutdownModule()
{
	FAxonGASBulkFillAdapter::Unregister();
	FAxonToolRegistry::Get().UnregisterNamespace(TEXT("gas"));
}

IMPLEMENT_MODULE(FAxonGASModule, AxonGAS)