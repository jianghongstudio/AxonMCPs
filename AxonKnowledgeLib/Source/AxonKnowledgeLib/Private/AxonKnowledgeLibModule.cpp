#include "AxonKnowledgeLibModule.h"
#include "AxonKnowledgeMetaActions.h"
#include "AxonCoreModule.h"
#include "AxonToolRegistry.h"

void FAxonKnowledgeLibModule::StartupModule()
{
	FAxonKnowledgeMetaActions::RegisterAll();
}

void FAxonKnowledgeLibModule::ShutdownModule()
{
	if (FAxonCoreModule::IsAvailable())
	{
		FAxonToolRegistry::Get().UnregisterNamespace(TEXT("knowledge"));
	}
}

IMPLEMENT_MODULE(FAxonKnowledgeLibModule, AxonKnowledgeLib)
