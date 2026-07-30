#pragma once
#include "CoreMinimal.h"
#include "AxonToolRegistry.h"

class FAxonBlueprintDiffActions
{
public:
	static void RegisterActions(FAxonToolRegistry& Registry);

	static FAxonActionResult HandleCompareBlueprints(const TSharedPtr<FJsonObject>& Params);
};
