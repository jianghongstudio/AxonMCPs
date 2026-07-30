#pragma once

#include "CoreMinimal.h"
#include "AxonToolRegistry.h"

/**
 * PIE-runtime animation telemetry actions for Axon.
 *
 * Modeled on FAxonLogicDriverRuntimeActions — resolves a live PIE actor's
 * USkeletalMeshComponent + UAnimInstance and reports live state (active state
 * machine state, montage, requested anim-instance variables, bone/socket
 * transforms). Read-only sampling; no graph mutation.
 */
class FAxonAnimationRuntimeActions
{
public:
	static void RegisterActions(FAxonToolRegistry& Registry);

private:
	static FAxonActionResult HandleSamplePIEAnimInstance(const TSharedPtr<FJsonObject>& Params);
};
