#include "AxonEditorModule.h"

#include "AxonCoreModule.h"
#include "AxonJsonUtils.h"
#include "AxonPieActions.h"
#include "AxonToolRegistry.h"

void FAxonEditorModule::StartupModule()
{
	FAxonPieActions::RegisterActions(FAxonToolRegistry::Get());
	UE_LOG(LogAxon, Verbose, TEXT("Axon Editor module loaded"));
}

void FAxonEditorModule::ShutdownModule()
{
	if (FAxonCoreModule::IsAvailable())
	{
		FAxonPieActions::UnregisterActions(FAxonToolRegistry::Get());
	}
}

IMPLEMENT_MODULE(FAxonEditorModule, AxonEditor)
