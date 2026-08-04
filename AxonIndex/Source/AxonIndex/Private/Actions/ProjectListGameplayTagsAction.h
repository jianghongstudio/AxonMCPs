#pragma once

#include "CoreMinimal.h"
#include "AxonToolRegistry.h"

class FProjectListGameplayTagsAction
{
public:
	static FAxonActionResult Execute(const TSharedPtr<FJsonObject>& Params);
	static FString GetName() { return TEXT("list_gameplay_tags"); }
	static FString GetDescription() { return TEXT("List all indexed gameplay tags, optionally filtered by prefix (e.g. 'Weapon.Melee')"); }
	static TSharedPtr<FJsonObject> GetSchema();
};
