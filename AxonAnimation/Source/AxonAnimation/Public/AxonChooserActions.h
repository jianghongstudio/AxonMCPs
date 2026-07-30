#pragma once

#include "CoreMinimal.h"
#include "AxonToolRegistry.h"

/**
 * Chooser table authoring actions for Axon — new `chooser` namespace,
 * registered from within the AxonAnimation module (no new module).
 *
 * 6 actions: inspect_chooser, duplicate_chooser_tree, set_context_object_class,
 * set_result_asset_reference, set_evaluate_chooser_result_reference, validate_chooser.
 *
 * Operates on UChooserTable assets (Chooser plugin). All handlers are gated behind
 * WITH_CHOOSER; when the Chooser plugin is absent the off-gate stub returns a clean
 * "Chooser plugin not available" error rather than failing to link.
 */
class AXONANIMATION_API FAxonChooserActions
{
public:
	/** Register all chooser actions with the tool registry. Always registers; gating is per-handler. */
	static void RegisterActions(FAxonToolRegistry& Registry);

private:
	static FAxonActionResult HandleInspectChooser(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleDuplicateChooserTree(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleSetContextObjectClass(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleSetResultAssetReference(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleSetEvaluateChooserResultReference(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleValidateChooser(const TSharedPtr<FJsonObject>& Params);
};
