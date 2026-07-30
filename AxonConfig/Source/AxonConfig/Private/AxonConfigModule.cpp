#include "AxonConfigModule.h"
#include "AxonConfigActions.h"
#include "AxonToolRegistry.h"
#include "AxonJsonUtils.h"

#define LOCTEXT_NAMESPACE "FAxonConfigModule"

void FAxonConfigModule::StartupModule()
{
	FAxonConfigActions::RegisterActions(FAxonToolRegistry::Get());
	UE_LOG(LogAxon, Log, TEXT("Axon — Config module loaded"));
}

void FAxonConfigModule::ShutdownModule()
{
	FAxonToolRegistry::Get().UnregisterNamespace(TEXT("config"));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FAxonConfigModule, AxonConfig)
