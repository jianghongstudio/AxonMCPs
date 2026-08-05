#pragma once

#include "CoreMinimal.h"
#include "AxonLLMSettings.h"

struct FAxonLlmRouteResult
{
	bool bOk = false;
	FString Error;
	FAxonLlmWorkerProfile Worker;
	/** Index into UAxonLLMSettings::Workers (stable id for this process config). */
	int32 WorkerIndex = INDEX_NONE;
};

class FAxonLlmRouter
{
public:
	/** Auto-pick first enabled worker that allows Scope (Workers array order = priority). */
	static FAxonLlmRouteResult Resolve(EAxonLlmScope Scope);

	static bool WorkerHasScope(const FAxonLlmWorkerProfile& Worker, EAxonLlmScope Scope);
};
