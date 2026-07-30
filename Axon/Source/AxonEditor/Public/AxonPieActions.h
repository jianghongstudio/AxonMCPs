#pragma once

#include "CoreMinimal.h"
#include "AxonToolRegistry.h"

class FAxonPieActions
{
public:
	static void RegisterActions(FAxonToolRegistry& Registry);
	static void UnregisterActions(FAxonToolRegistry& Registry);

	static FAxonActionResult HandleSamplePieTimeseries(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandlePollPieSmoke(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleStopPieSmoke(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleStartPie(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleStopPie(const TSharedPtr<FJsonObject>& Params);
};
