#pragma once

#include "CoreMinimal.h"
#include "AxonToolRegistry.h"

class FProjectFindByTypeAction
{
public:
	static FAxonActionResult Execute(const TSharedPtr<FJsonObject>& Params);
	static FString GetName() { return TEXT("find_by_type"); }
	static FString GetDescription() { return TEXT("Find all assets of a given type (e.g. Blueprint, Material, StaticMesh)"); }
	static TSharedPtr<FJsonObject> GetSchema();
};
