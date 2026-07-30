#pragma once

#include "CoreMinimal.h"
#include "AxonToolRegistry.h"

/**
 * MirrorDataTable domain action handlers for Axon (Motion Matching Pack — Sprint 2).
 * - create_mirror_data_table     : create a UMirrorDataTable, populate find/replace rules, build rows.
 * - set_schema_mirror_data_table : assign a mirror table to a PoseSearchSchema roled-skeleton slot.
 */
class AXONANIMATION_API FAxonMirrorTableActions
{
public:
	/** Register all mirror-table actions with the tool registry */
	static void RegisterActions(FAxonToolRegistry& Registry);

private:
	static FAxonActionResult HandleCreateMirrorDataTable(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleSetSchemaMirrorDataTable(const TSharedPtr<FJsonObject>& Params);
};
