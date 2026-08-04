#pragma once

#include "CoreMinimal.h"
#include "AxonToolRegistry.h"

class FProjectGetSavedAssetStateAction
{
public:
	static FAxonActionResult Execute(const TSharedPtr<FJsonObject>& Params);
	static FString GetName() { return TEXT("get_saved_asset_state"); }
	static FString GetDescription() { return TEXT("Return disk-backed state for an asset -- class, package, disk path, file size, mtime, dependencies and referencers"); }
	static TSharedPtr<FJsonObject> GetSchema();
};
