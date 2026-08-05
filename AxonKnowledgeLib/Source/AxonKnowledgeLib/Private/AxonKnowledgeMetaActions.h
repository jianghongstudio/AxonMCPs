#pragma once

#include "CoreMinimal.h"
#include "AxonToolRegistry.h"

/** Meta actions on the "knowledge" namespace (scaffold + name preview). */
class FAxonKnowledgeMetaActions
{
public:
	static void RegisterAll();

	static FAxonActionResult HandleScaffoldKbPlugin(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandlePreviewKbNames(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleListKbPacks(const TSharedPtr<FJsonObject>& Params);
};
