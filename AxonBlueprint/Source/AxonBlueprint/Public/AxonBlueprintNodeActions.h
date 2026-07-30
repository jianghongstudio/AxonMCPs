#pragma once
#include "CoreMinimal.h"
#include "AxonToolRegistry.h"

class FAxonBlueprintNodeActions
{
public:
	static void RegisterActions(FAxonToolRegistry& Registry);

	static FAxonActionResult HandleAddNode(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleRemoveNode(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleConnectPins(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleDisconnectPins(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleSetPinDefault(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleSetNodePosition(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleBatchExecute(const TSharedPtr<FJsonObject>& Params);

	// Wave 3 — Discovery & Resolution
	static FAxonActionResult HandleResolveNode(const TSharedPtr<FJsonObject>& Params);

	// Wave 4 — Bulk Node Operations
	static FAxonActionResult HandleAddNodesBulk(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleConnectPinsBulk(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleSetPinDefaultsBulk(const TSharedPtr<FJsonObject>& Params);

	// Wave 5 — Scaffolding & Templates
	static FAxonActionResult HandleAddTimeline(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleAddEventNode(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleAddCommentNode(const TSharedPtr<FJsonObject>& Params);

	// Phase 3A — Timeline read/edit
	static FAxonActionResult HandleGetTimelineData(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleAddTimelineTrack(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleSetTimelineKeys(const TSharedPtr<FJsonObject>& Params);

	// Wave 7 — Advanced
	static FAxonActionResult HandlePromotePinToVariable(const TSharedPtr<FJsonObject>& Params);

	// Phase 1 (gap #11) — Cross-class property access (foreign-class VariableGet/Set)
	static FAxonActionResult HandleAddPropertyAccess(const TSharedPtr<FJsonObject>& Params);

	// Genuine thread-safe Property Access — reflective UK2Node_PropertyAccess authoring.
	// Unlike add_property_access (which emits a foreign-member VariableGet with a self
	// pin, non-thread-safe), this spawns a real K2Node_PropertyAccess whose path-based
	// read is resolved thread-safe (or game-thread-cached) by the AnimBP property-access
	// compiler. The class is MinimalAPI/unlinkable, so it is created reflectively.
	static FAxonActionResult HandleAddPropertyAccessNode(const TSharedPtr<FJsonObject>& Params);
};
