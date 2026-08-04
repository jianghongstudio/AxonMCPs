#pragma once

#include "CoreMinimal.h"

/**
 * Registers route/search/read/list_topics (+ optional extract_*) on a KB namespace
 * backed by Knowledge/ under the given sibling plugin.
 */
class AXONKNOWLEDGELIB_API FAxonKnowledgeRegistration
{
public:
	struct FOptions
	{
		bool bIncludeExtract = true;
	};

	static void RegisterAll(const FString& Namespace, const FString& PluginName, const FOptions& Options = FOptions());
	static void RegisterQueryActions(const FString& Namespace, const FString& PluginName);
	static void RegisterExtractActions(const FString& Namespace, const FString& PluginName);
};
