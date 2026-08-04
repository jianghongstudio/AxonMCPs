#pragma once

#include "CoreMinimal.h"
#include "AxonToolRegistry.h"

class FProjectExportAssetTextAction
{
public:
	static FAxonActionResult Execute(const TSharedPtr<FJsonObject>& Params);
	static FString GetName() { return TEXT("export_asset_text"); }
	static FString GetDescription();
	static TSharedPtr<FJsonObject> GetSchema();
};
