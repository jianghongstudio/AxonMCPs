#pragma once

#include "CoreMinimal.h"
#include "AxonLLMSettings.h"

class FJsonObject;

class FAxonLlmUsage
{
public:
	static void Record(
		EAxonLlmScope Scope,
		int32 WorkerIndex,
		const FString& Model,
		int32 CharsIn,
		int32 CharsOut,
		double LatencyMs,
		bool bOk,
		const FString& JobId);

	static TSharedPtr<FJsonObject> BuildSummary(int32 LastDays);
};
