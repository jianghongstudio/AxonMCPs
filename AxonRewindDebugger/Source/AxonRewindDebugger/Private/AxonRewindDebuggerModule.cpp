#include "AxonRewindDebuggerModule.h"
#include "AxonRewindDebuggerActions.h"
#include "AxonJsonUtils.h"
#include "AxonToolRegistry.h"

void FAxonRewindDebuggerModule::StartupModule()
{
	FAxonRewindDebuggerActions::RegisterActions(FAxonToolRegistry::Get());
	UE_LOG(LogAxon, Verbose, TEXT("Axon - RewindDebugger module loaded"));
}

void FAxonRewindDebuggerModule::ShutdownModule()
{
	FAxonToolRegistry::Get().UnregisterNamespace(TEXT("rewind_debugger"));
}

IMPLEMENT_MODULE(FAxonRewindDebuggerModule, AxonRewindDebugger)
