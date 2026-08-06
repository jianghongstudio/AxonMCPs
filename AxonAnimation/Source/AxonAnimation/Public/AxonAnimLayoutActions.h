#pragma once

#include "CoreMinimal.h"
#include "AxonToolRegistry.h"

/**
 * Animation Blueprint auto-layout action for Axon.
 * Uses IAxonGraphFormatter (Blueprint Assist bridge) to format ABP graphs.
 */
class AXONANIMATION_API FAxonAnimLayoutActions
{
public:
	/** Register all layout actions with the tool registry */
	static void RegisterActions(FAxonToolRegistry& Registry);

private:
	static FAxonActionResult HandleAutoLayout(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleGetAnimNodePositions(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleSetAnimNodePosition(const TSharedPtr<FJsonObject>& Params);
};
