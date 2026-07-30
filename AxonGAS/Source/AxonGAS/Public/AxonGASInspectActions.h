#pragma once

#include "AxonGASInternal.h"

class FAxonGASInspectActions
{
public:
	static void RegisterActions(FAxonToolRegistry& Registry);

private:
	// Phase 3: export_gas_manifest (moved from Phase 4 — operates on assets, not runtime)
	static FAxonActionResult HandleExportGASManifest(const TSharedPtr<FJsonObject>& Params);
	// Phase 4: Runtime Debug
	static FAxonActionResult HandleSnapshotGASState(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleGetTagState(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleGetCooldownState(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleTraceAbilityActivation(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleCompareGASStates(const TSharedPtr<FJsonObject>& Params);
};
