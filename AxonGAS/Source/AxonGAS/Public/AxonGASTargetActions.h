#pragma once

#include "AxonGASInternal.h"

class FAxonGASTargetActions
{
public:
	static void RegisterActions(FAxonToolRegistry& Registry);

private:
	// Phase 3: Targeting
	static FAxonActionResult HandleCreateTargetActor(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleConfigureTargetActor(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleAddTargetingToAbility(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleScaffoldFPSTargeting(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleValidateTargeting(const TSharedPtr<FJsonObject>& Params);
};
