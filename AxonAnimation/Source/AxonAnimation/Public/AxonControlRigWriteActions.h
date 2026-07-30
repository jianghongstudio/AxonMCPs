#pragma once

#include "CoreMinimal.h"
#include "AxonToolRegistry.h"

/**
 * Control Rig graph read/write action handlers for Axon.
 * 3 actions: graph reading, node spawning, pin wiring.
 */
class AXONANIMATION_API FAxonControlRigWriteActions
{
public:
	/** Register all Control Rig graph actions with the tool registry */
	static void RegisterActions(FAxonToolRegistry& Registry);

private:
	static FAxonActionResult HandleGetControlRigGraph(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleAddControlRigNode(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleConnectControlRigPins(const TSharedPtr<FJsonObject>& Params);
};
