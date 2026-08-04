#pragma once

#include "CoreMinimal.h"
#include "AxonToolRegistry.h"

class FProjectGetStatsAction
{
public:
	static FAxonActionResult Execute(const TSharedPtr<FJsonObject>& Params);
	static FString GetName() { return TEXT("get_stats"); }
	static FString GetDescription() { return TEXT("Get project index statistics -- total counts by table and asset class breakdown"); }
	static TSharedPtr<FJsonObject> GetSchema();
};
