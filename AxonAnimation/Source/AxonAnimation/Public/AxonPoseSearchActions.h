#pragma once

#include "CoreMinimal.h"
#include "AxonToolRegistry.h"

/**
 * PoseSearch domain action handlers for Axon.
 * 5 actions: schema inspection, database CRUD, stats.
 */
class AXONANIMATION_API FAxonPoseSearchActions
{
public:
	/** Register all PoseSearch actions with the tool registry */
	static void RegisterActions(FAxonToolRegistry& Registry);

private:
	static FAxonActionResult HandleGetPoseSearchSchema(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleGetPoseSearchDatabase(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleAddDatabaseSequence(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleRemoveDatabaseSequence(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleGetDatabaseStats(const TSharedPtr<FJsonObject>& Params);

	// --- Wave 11: PoseSearch Creation (2) ---
	static FAxonActionResult HandleCreatePoseSearchSchema(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleCreatePoseSearchDatabase(const TSharedPtr<FJsonObject>& Params);

	// --- Wave 14: PoseSearch Writes (6) ---
	static FAxonActionResult HandleSetDatabaseSequenceProperties(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleAddSchemaChannel(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleRemoveSchemaChannel(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleSetChannelWeight(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleRebuildPoseSearchIndex(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleSetDatabaseSearchMode(const TSharedPtr<FJsonObject>& Params);

	// --- Motion Matching Pack Sprint 1: NormalizationSet + DB entry tags (3 class-member) ---
	static FAxonActionResult HandleCreateNormalizationSet(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleAddDatabaseToNormalizationSet(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleSetDatabaseEntryTags(const TSharedPtr<FJsonObject>& Params);
};
