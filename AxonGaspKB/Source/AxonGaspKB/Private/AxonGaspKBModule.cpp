#include "AxonGaspKBModule.h"
#include "AxonKnowledgeRegistration.h"

void FAxonGaspKBModule::StartupModule()
{
	// Historical namespace gasp_kb (not game_animation_sample_kb).
	FAxonKnowledgeRegistration::RegisterAll(TEXT("gasp_kb"), TEXT("AxonGaspKB"));
}

void FAxonGaspKBModule::ShutdownModule()
{
	FAxonKnowledgeRegistration::UnregisterAll(TEXT("gasp_kb"), TEXT("AxonGaspKB"));
}

IMPLEMENT_MODULE(FAxonGaspKBModule, AxonGaspKB)
