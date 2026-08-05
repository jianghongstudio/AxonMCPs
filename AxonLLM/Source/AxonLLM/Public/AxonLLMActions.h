#pragma once

#include "CoreMinimal.h"
#include "AxonToolRegistry.h"

class FAxonLLMActions
{
public:
	static void RegisterActions(FAxonToolRegistry& Registry);

	static FAxonActionResult HandleStatus(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleList(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleRun(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleRunAsync(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleJobStatus(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleJobCancel(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleUsageSummary(const TSharedPtr<FJsonObject>& Params);
};
