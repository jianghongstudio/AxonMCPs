#pragma once

#include "CoreMinimal.h"
#include "AxonToolRegistry.h"

class FProjectSearchGameplayTagsAction
{
public:
	static FAxonActionResult Execute(const TSharedPtr<FJsonObject>& Params);
	static FString GetName() { return TEXT("search_gameplay_tags"); }
	static FString GetDescription() { return TEXT("Search gameplay tags by substring and return matching tags with their referencing assets"); }
	static TSharedPtr<FJsonObject> GetSchema();
};
