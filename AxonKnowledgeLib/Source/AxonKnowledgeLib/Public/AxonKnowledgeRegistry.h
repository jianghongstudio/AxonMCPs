#pragma once

#include "CoreMinimal.h"

/** One KB pack that called FAxonKnowledgeRegistration::RegisterAll at module startup. */
struct AXONKNOWLEDGELIB_API FAxonKnowledgePackInfo
{
	FString PluginName;
	FString Namespace;
};

/**
 * Runtime index of knowledge packs. Packs register themselves via RegisterAll;
 * consumers (AxonLLM settings, Agents) query this instead of guessing plugin names.
 */
class AXONKNOWLEDGELIB_API FAxonKnowledgeRegistry
{
public:
	static void Register(const FString& PluginName, const FString& Namespace);
	static void Unregister(const FString& PluginName);

	static TArray<FAxonKnowledgePackInfo> GetRegistered();
	static TArray<FString> GetRegisteredPluginNames();
	static bool IsRegistered(const FString& PluginName);
};
