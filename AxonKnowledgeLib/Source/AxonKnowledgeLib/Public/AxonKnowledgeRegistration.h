#pragma once

#include "CoreMinimal.h"

/**
 * Registers route/search/read/list_topics (+ optional extract_*) on a KB namespace
 * backed by Knowledge/ under the given sibling plugin.
 * Also indexes the pack in FAxonKnowledgeRegistry for discovery (AxonLLM, etc.).
 */
class AXONKNOWLEDGELIB_API FAxonKnowledgeRegistration
{
public:
	struct FOptions
	{
		bool bIncludeExtract = true;
	};

	static void RegisterAll(const FString& Namespace, const FString& PluginName, const FOptions& Options = FOptions());
	/** Remove MCP namespace actions and drop the pack from FAxonKnowledgeRegistry. */
	static void UnregisterAll(const FString& Namespace, const FString& PluginName);
	static void RegisterQueryActions(const FString& Namespace, const FString& PluginName);
	static void RegisterExtractActions(const FString& Namespace, const FString& PluginName);
};
