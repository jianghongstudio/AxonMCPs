#pragma once

#include "CoreMinimal.h"
#include "AxonToolRegistry.h"

class FProjectSearchAction
{
public:
	static FAxonActionResult Execute(const TSharedPtr<FJsonObject>& Params);
	static FString GetName() { return TEXT("search"); }
	static FString GetDescription() { return TEXT("Full-text search across all indexed project assets, nodes, variables, and parameters"); }
	static TSharedPtr<FJsonObject> GetSchema();
};
