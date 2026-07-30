#pragma once

#include "CoreMinimal.h"
#include "AxonToolRegistry.h"

/**
 * Core discovery, status, and extension-scaffold tool implementations.
 * Registered under the "axon" namespace (expanded to axon_* tools).
 */
class FAxonCoreTools
{
public:
	static void RegisterAll();

	static FAxonActionResult HandleDiscover(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleStatus(const TSharedPtr<FJsonObject>& Params);

	static FAxonActionResult HandleResearchExtensionTarget(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleCreateExtension(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleListExtensionRecipes(const TSharedPtr<FJsonObject>& Params);
};
