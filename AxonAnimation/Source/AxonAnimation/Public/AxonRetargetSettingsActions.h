#pragma once

#include "CoreMinimal.h"
#include "AxonToolRegistry.h"

class UIKRetargeterController;
class UIKRetargetOpControllerBase;
class UScriptStruct;

/**
 * IK Retargeter op-stack + retarget-pose authoring actions for Axon (animation namespace).
 *
 * Tier-1 retarget surface (plan 2026-06-16-animation-retargeting-mcp-actions.md):
 *   - align_retarget_pose            (T1-R1) AutoAlignAllBones / AutoAlignBones / SnapBoneToGround
 *   - get_retarget_pose              (T1-R2) read per-bone rotation deltas + root translation delta
 *   - set_retarget_pose              (T1-R2) from_reference + bone_deltas[] (from_animation DEFERRED in v1)
 *   - get_retarget_chain_settings    (T1-R3) FK/IK chains op controller GetSettings()
 *   - set_retarget_chain_settings    (T1-R3) FK/IK chains op controller SetSettings()
 *   - set_retarget_root_settings     (T1-R5) Pelvis Motion op controller settings
 *   - enable_foot_ground_lock        (T1-R5) Speed Planting op chains + SnapBoneToGround
 *
 * The 5.6 IK Retargeter refactor exposes op settings through per-op BlueprintCallable
 * controller UClasses reached via UIKRetargeterController::GetOpController(int32) ->
 * cast to the op-specific controller -> Get/SetSettings(). Op settings are returned
 * BY VALUE (copy-mutate-set). The deprecated monolithic settings structs in
 * IKRetargetDeprecated.h are OFF-LIMITS. EFKChainRotationMode has non-sequential
 * explicit enum values and is parsed by name, never by raw int.
 */
class AXONANIMATION_API FAxonRetargetSettingsActions
{
public:
	/** Register all Tier-1 retarget op-stack + pose actions with the tool registry. */
	static void RegisterActions(FAxonToolRegistry& Registry);

private:
	// T1-R1 — auto-align retarget pose / snap-to-ground
	static FAxonActionResult HandleAlignRetargetPose(const TSharedPtr<FJsonObject>& Params);

	// T1-R2 — retarget pose read/write (v1: from_reference + bone_deltas[]; from_animation DEFERRED)
	static FAxonActionResult HandleGetRetargetPose(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleSetRetargetPose(const TSharedPtr<FJsonObject>& Params);

	// T1-R3 — FK/IK chains op settings read/write
	static FAxonActionResult HandleGetRetargetChainSettings(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleSetRetargetChainSettings(const TSharedPtr<FJsonObject>& Params);

	// T1-R5 — Pelvis Motion op settings + Speed Planting / ground-lock
	static FAxonActionResult HandleSetRetargetRootSettings(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleEnableFootGroundLock(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Resolve the op controller for the first op in the stack whose op-type UScriptStruct
	 * matches OpType. Returns the base controller (cast to the op-specific controller at the
	 * call site) and writes the op's stack index to OutOpIndex. Returns nullptr when no op of
	 * that type is present in the stack (caller surfaces a clear "op not in stack" error).
	 */
	static UIKRetargetOpControllerBase* ResolveOpController(
		UIKRetargeterController* Controller,
		const UScriptStruct* OpType,
		int32& OutOpIndex);
};
