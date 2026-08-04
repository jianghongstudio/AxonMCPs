#include "AxonGaspKBModule.h"
#include "AxonKnowledgeRegistration.h"
#include "AxonCoreModule.h"
#include "AxonToolRegistry.h"

void FAxonGaspKBModule::StartupModule()
{
	// Historical namespace gasp_kb (not game_animation_sample_kb).
	FAxonKnowledgeRegistration::RegisterAll(TEXT("gasp_kb"), TEXT("AxonGaspKB"));
}

void FAxonGaspKBModule::ShutdownModule()
{
	if (FAxonCoreModule::IsAvailable())
	{
		FAxonToolRegistry::Get().UnregisterNamespace(TEXT("gasp_kb"));
	}
}

IMPLEMENT_MODULE(FAxonGaspKBModule, AxonGaspKB)
