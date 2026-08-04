#include "AxonSourceModule.h"
#include "AxonSourceActions.h"
#include "AxonToolRegistry.h"
#include "AxonSettings.h"
#include "AxonJsonUtils.h"
#include "AxonCoreModule.h"

#define LOCTEXT_NAMESPACE "FAxonSourceModule"

void FAxonSourceModule::StartupModule()
{
	if (!GetDefault<UAxonSettings>()->bEnableSource) return;

	FAxonSourceActions::RegisterAll();
	UE_LOG(LogAxon, Log, TEXT("Axon — Source module loaded (18 actions)"));
}

void FAxonSourceModule::ShutdownModule()
{
	if (FAxonCoreModule::IsAvailable())
	{
		FAxonToolRegistry::Get().UnregisterNamespace(TEXT("source"));
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FAxonSourceModule, AxonSource)
