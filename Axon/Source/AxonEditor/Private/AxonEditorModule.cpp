#include "AxonEditorModule.h"

#include "AxonCoreModule.h"
#include "AxonEditorActions.h"
#include "AxonEditorMapActions.h"
#include "AxonJsonUtils.h"
#include "AxonPieActions.h"
#include "AxonPieInputActions.h"
#include "AxonPieObjectActions.h"
#include "AxonStatActions.h"
#include "AxonToolRegistry.h"
#include "Misc/OutputDeviceRedirector.h"

namespace
{
	FAxonLogCapture* GAxonLogCapture = nullptr;
}

void FAxonEditorModule::StartupModule()
{
	GAxonLogCapture = new FAxonLogCapture();
	GLog->AddOutputDevice(GAxonLogCapture);

	FAxonToolRegistry& Registry = FAxonToolRegistry::Get();
	FAxonPieActions::RegisterActions(Registry);
	FAxonEditorActions::RegisterActions(GAxonLogCapture);
	FAxonEditorMapActions::RegisterActions(Registry);
	FAxonPieObjectActions::RegisterActions(Registry);
	FAxonPieInputActions::RegisterActions(Registry);
	FAxonPieInputActions::RegisterPieEndHook();
	FAxonStatActions::RegisterActions(Registry);

	UE_LOG(LogAxon, Log, TEXT("Axon Editor module loaded (%d editor actions)"), Registry.GetActions(TEXT("editor")).Num());
}

void FAxonEditorModule::ShutdownModule()
{
	if (!FAxonCoreModule::IsAvailable())
	{
		return;
	}

	FAxonPieInputActions::UnregisterPieEndHook();
	FAxonToolRegistry& Registry = FAxonToolRegistry::Get();
	Registry.UnregisterNamespace(TEXT("editor"));
	Registry.UnregisterAction(TEXT("animation"), TEXT("sample_pie_timeseries"));

	if (GAxonLogCapture)
	{
		GLog->RemoveOutputDevice(GAxonLogCapture);
		delete GAxonLogCapture;
		GAxonLogCapture = nullptr;
	}
}

IMPLEMENT_MODULE(FAxonEditorModule, AxonEditor)