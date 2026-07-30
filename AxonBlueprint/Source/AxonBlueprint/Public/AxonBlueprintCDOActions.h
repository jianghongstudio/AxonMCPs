#pragma once
#include "CoreMinimal.h"
#include "AxonToolRegistry.h"

class FAxonBlueprintCDOActions
{
public:
	static void RegisterActions(FAxonToolRegistry& Registry);

	static FAxonActionResult HandleGetCDOProperties(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleSetCDOProperty(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleSetPropertyAtPath(const TSharedPtr<FJsonObject>& Params);
};
