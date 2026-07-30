#pragma once
#include "CoreMinimal.h"
#include "AxonToolRegistry.h"

class FAxonBlueprintGraphExportActions
{
public:
	static void RegisterActions(FAxonToolRegistry& Registry);

	// Phase 5C — Graph export/import/copy
	static FAxonActionResult HandleExportGraph(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleCopyNodes(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleDuplicateGraph(const TSharedPtr<FJsonObject>& Params);
};
