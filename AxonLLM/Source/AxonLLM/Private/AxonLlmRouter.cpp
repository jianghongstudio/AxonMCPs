#include "AxonLlmRouter.h"

bool FAxonLlmRouter::WorkerHasScope(const FAxonLlmWorkerProfile& Worker, EAxonLlmScope Scope)
{
	return Worker.AllowsScope(Scope);
}

FAxonLlmRouteResult FAxonLlmRouter::Resolve(EAxonLlmScope Scope)
{
	FAxonLlmRouteResult Out;
	const UAxonLLMSettings* Settings = UAxonLLMSettings::Get();
	if (!Settings)
	{
		Out.Error = TEXT("AxonLLM settings unavailable");
		return Out;
	}

	const bool bNeedsLlmEndpoint = Scope != EAxonLlmScope::KnowledgePromoteDraft;

	for (int32 Index = 0; Index < Settings->Workers.Num(); ++Index)
	{
		const FAxonLlmWorkerProfile& W = Settings->Workers[Index];
		if (!W.bEnabled)
		{
			continue;
		}
		if (bNeedsLlmEndpoint && (W.BaseUrl.IsEmpty() || W.Model.IsEmpty()))
		{
			continue;
		}
		if (!WorkerHasScope(W, Scope))
		{
			continue;
		}
		Out.bOk = true;
		Out.Worker = W;
		Out.WorkerIndex = Index;
		return Out;
	}

	Out.Error = FString::Printf(
		TEXT("No enabled worker is authorized for scope '%s'. Enable the matching checkbox under Workers → 能力."),
		*UAxonLLMSettings::ScopeToWire(Scope));
	return Out;
}
