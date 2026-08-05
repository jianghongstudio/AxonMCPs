#include "AxonLLMModule.h"
#include "AxonLLMActions.h"
#include "AxonLlmJobQueue.h"
#include "AxonLlmWorkerProfileCustomization.h"
#include "AxonToolRegistry.h"
#include "AxonCoreModule.h"
#include "AxonJsonUtils.h"
#include "PropertyEditorModule.h"
#include "AxonLLMSettings.h"

#define LOCTEXT_NAMESPACE "FAxonLLMModule"

void FAxonLLMModule::StartupModule()
{
	FAxonLLMActions::RegisterActions(FAxonToolRegistry::Get());

	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.RegisterCustomPropertyTypeLayout(
		FAxonLlmWorkerProfile::StaticStruct()->GetFName(),
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FAxonLlmWorkerProfileCustomization::MakeInstance));

	if (FAxonCoreModule::IsAvailable())
	{
		FAxonCoreModule::Get().SetWorkerHudProvider([]()
		{
			return FAxonLlmJobQueue::Get().GetBusySnapshot();
		});
	}

	UE_LOG(LogAxon, Log, TEXT("Axon — LLM worker module loaded (namespace: worker)"));
}

void FAxonLLMModule::ShutdownModule()
{
	if (FAxonCoreModule::IsAvailable())
	{
		FAxonCoreModule::Get().SetWorkerHudProvider(nullptr);
		FAxonToolRegistry::Get().UnregisterNamespace(TEXT("worker"));
	}

	FAxonLlmJobQueue::Get().Shutdown();

	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.UnregisterCustomPropertyTypeLayout(FAxonLlmWorkerProfile::StaticStruct()->GetFName());
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FAxonLLMModule, AxonLLM)
