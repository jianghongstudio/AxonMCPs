#pragma once

#include "CoreMinimal.h"
#include "AxonToolRegistry.h"

class FProjectFindReferencesAction
{
public:
	static FAxonActionResult Execute(const TSharedPtr<FJsonObject>& Params);
	static FString GetName() { return TEXT("find_references"); }
	static FString GetDescription() { return TEXT("Find all assets that reference or are referenced by the given asset"); }
	static TSharedPtr<FJsonObject> GetSchema();
};
