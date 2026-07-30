#pragma once

#include "CoreMinimal.h"
#include "AxonToolRegistry.h"

class UAnimMontage;
class UBlendSpace;
class UAnimBlueprint;
class UAnimSequence;
class USkeleton;
class USkeletalMesh;

/**
 * Animation domain action handlers for Axon.
 * Ported from AnimationMCPReaderLibrary — 23 proven actions.
 */
class FAxonAnimationActions
{
public:
	/** Register all animation actions with the tool registry */
	static void RegisterActions(FAxonToolRegistry& Registry);

	// --- Montage Sections (4) ---
	static FAxonActionResult HandleAddMontageSection(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleDeleteMontageSection(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleSetSectionNext(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleSetSectionTime(const TSharedPtr<FJsonObject>& Params);

	// --- BlendSpace Samples (5) ---
	static FAxonActionResult HandleAddBlendSpaceSample(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleEditBlendSpaceSample(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleDeleteBlendSpaceSample(const TSharedPtr<FJsonObject>& Params);
	// Standalone resample+dirty for already-broken / externally-authored blend spaces.
	static FAxonActionResult HandleBakeBlendSpace(const TSharedPtr<FJsonObject>& Params);
	// Sets bInterpolateUsingGrid + PreferredTriangulationDirection, then rebakes.
	static FAxonActionResult HandleSetBlendSpaceInterpolation(const TSharedPtr<FJsonObject>& Params);

	// --- ABP Graph Reading (7) ---
	static FAxonActionResult HandleGetStateMachines(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleGetStateInfo(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleGetTransitions(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleGetBlendNodes(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleGetLinkedLayers(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleGetGraphs(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleGetNodes(const TSharedPtr<FJsonObject>& Params);
	// Enumerate AnimGraph EvaluateChooser (v1/v2) nodes + reflectively-resolved chooser asset;
	// optional recursive nested-tree expansion via the Phase-2 AxonChooserTreeCollector.
	static FAxonActionResult HandleGetAnimGraphChoosers(const TSharedPtr<FJsonObject>& Params);

	// --- Notify Editing (2) ---
	static FAxonActionResult HandleSetNotifyTime(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleSetNotifyDuration(const TSharedPtr<FJsonObject>& Params);

	// --- Bone Tracks (3) ---
	static FAxonActionResult HandleSetBoneTrackKeys(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleAddBoneTrack(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleRemoveBoneTrack(const TSharedPtr<FJsonObject>& Params);

	// --- Bone Pose Copy (1) ---
	// Reads the evaluated pose (raw track + ref pose fallback) for a list of
	// bones at a given time on the source AnimSequence, then writes those
	// transforms as keys to the destination AnimSequence. Useful when a target
	// anim has T-pose / wrong values on a subset of bones (e.g. left arm) and
	// you want to import a clean pose from a working anim without touching
	// the rest of the target.
	static FAxonActionResult HandleCopyBonePoseBetweenSequences(const TSharedPtr<FJsonObject>& Params);

	// --- Virtual Bones (2) ---
	static FAxonActionResult HandleAddVirtualBone(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleRemoveVirtualBones(const TSharedPtr<FJsonObject>& Params);

	// --- Skeleton Info (2) ---
	static FAxonActionResult HandleGetSkeletonInfo(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleGetSkeletalMeshInfo(const TSharedPtr<FJsonObject>& Params);

	// --- Wave 1: Read Actions (8) ---
	static FAxonActionResult HandleGetSequenceInfo(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleGetSequenceNotifies(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleGetBoneTrackKeys(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleListBoneTracks(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleGetSequenceCurves(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleGetMontageInfo(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleGetBlendSpaceInfo(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleGetSkeletonSockets(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleGetSkeletonPreviewAttachedAssets(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleGetBoneRefPose(const TSharedPtr<FJsonObject>& Params);
	// T3-2: animated-frame FK-composed transform for one bone (extends get_bone_ref_pose's
	// bind-pose compose to an evaluated animation frame/time).
	static FAxonActionResult HandleGetAnimatedBoneTransform(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleGetAbpInfo(const TSharedPtr<FJsonObject>& Params);

	// --- Wave 2: Notify CRUD (4) ---
	static FAxonActionResult HandleAddNotify(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleAddNotifyState(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleRemoveNotify(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleSetNotifyTrack(const TSharedPtr<FJsonObject>& Params);

	// --- Wave 3: Curve CRUD (5) ---
	static FAxonActionResult HandleListCurves(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleAddCurve(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleRemoveCurve(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleSetCurveKeys(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleGetCurveKeys(const TSharedPtr<FJsonObject>& Params);

	// --- Wave 4: Skeleton + BlendSpace (6) ---
	static FAxonActionResult HandleAddSocket(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleRemoveSocket(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleSetSocketTransform(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleGetSkeletonCurves(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleSetBlendSpaceAxis(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleSetRootMotionSettings(const TSharedPtr<FJsonObject>& Params);

	// --- Wave 5: Creation + Montage (6) ---
	static FAxonActionResult HandleCreateSequence(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleDuplicateSequence(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleCreateMontage(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleSetMontageBlend(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleAddMontageSlot(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleSetMontageSlot(const TSharedPtr<FJsonObject>& Params);

	// --- Wave 7: Anim Modifiers + Composites (5) ---
	static FAxonActionResult HandleApplyAnimModifier(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleListAnimModifiers(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleGetCompositeInfo(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleAddCompositeSegment(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleRemoveCompositeSegment(const TSharedPtr<FJsonObject>& Params);

	// --- Wave 8a: IKRig (5) ---
	static FAxonActionResult HandleGetIKRigInfo(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleAddIKSolver(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleRemoveIKSolver(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleGetRetargeterInfo(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleSetRetargetChainMapping(const TSharedPtr<FJsonObject>& Params);

	// --- Wave 8b: Control Rig Read (2) ---
	static FAxonActionResult HandleGetControlRigInfo(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleGetControlRigVariables(const TSharedPtr<FJsonObject>& Params);

	// --- Wave 8c: Control Rig Write (1) ---
	static FAxonActionResult HandleAddControlRigElement(const TSharedPtr<FJsonObject>& Params);

	// --- Wave 9: ABP Read Enhancements (2) ---
	static FAxonActionResult HandleGetAbpVariables(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleGetAbpLinkedAssets(const TSharedPtr<FJsonObject>& Params);

	// --- Skeleton Compatibility (3) ---
	// Wraps USkeleton::CompatibleSkeletons — required for playing UE4 mannequin
	// anims on UE5 SK_Mannequin meshes (and similar legacy/cross-skeleton flows)
	// without manual run_python boilerplate.
	static FAxonActionResult HandleGetCompatibleSkeletons(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleAddCompatibleSkeleton(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleRemoveCompatibleSkeleton(const TSharedPtr<FJsonObject>& Params);

	// --- Wave 10: ABP Write Experimental (3) ---
	static FAxonActionResult HandleAddStateToMachine(const TSharedPtr<FJsonObject>& Params);
	// add_conduit — spawn a UAnimStateConduitNode (a transition hub) into a state
	// machine. Its BoundGraph is a transition-logic graph, NOT an anim/pose graph.
	static FAxonActionResult HandleAddConduit(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleAddTransition(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleSetTransitionRule(const TSharedPtr<FJsonObject>& Params);
	// Phase 6 — read back a transition's authored rule (kind + operands + comparison).
	static FAxonActionResult HandleGetTransitionRule(const TSharedPtr<FJsonObject>& Params);

	// --- State-machine editing: removal + entry re-point ---
	// State/transition nodes own a BoundGraph, so removal routes through
	// FBlueprintEditorUtils::RemoveNode -> DestroyNode (tears down the inner graph)
	// rather than a bare UEdGraph::RemoveNode.
	static FAxonActionResult HandleRemoveAnimState(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleSetAnimEntryState(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleRemoveAnimTransition(const TSharedPtr<FJsonObject>& Params);

	// --- Wave 16: State Machine Authoring (#13/#14) ---
	// create_state_machine — spawn a UAnimGraphNode_StateMachine into an ABP's anim graph.
	// build_state_machine  — declarative builder composing create + add states/transitions/rules.
	static FAxonActionResult HandleCreateStateMachine(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleBuildStateMachine(const TSharedPtr<FJsonObject>& Params);

	// --- Wave 14: Notify Properties (1) ---
	static FAxonActionResult HandleSetNotifyProperties(const TSharedPtr<FJsonObject>& Params);

	// --- Wave 15: Physics Assets + IK Chains (6) ---
	static FAxonActionResult HandleGetPhysicsAssetInfo(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleSetBodyProperties(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleSetConstraintProperties(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleAddRetargetChain(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleRemoveRetargetChain(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleSetRetargetChainBones(const TSharedPtr<FJsonObject>& Params);

	// --- Retarget CREATE/RUN pack (4) ---
	// Creates the IK Rig + IK Retargeter assets and runs a cross-skeleton batch
	// retarget. Complements the existing IK Rig / retargeter MUTATION actions
	// (get_ikrig_info, add_ik_solver, get_retargeter_info, set_retarget_chain_mapping,
	// add/remove/set_retarget_chain*) which only operate on assets that already exist.
	static FAxonActionResult HandleCreateIKRig(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleCreateIKRetargeter(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleSetRetargeterRigs(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleBatchRetargetAnimations(const TSharedPtr<FJsonObject>& Params);

	// --- Wave 11: Asset Creation + Setup (7 in this file) ---
	static FAxonActionResult HandleCreateBlendSpace(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleCreateBlendSpace1D(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleCreateAimOffset(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleCreateAimOffset1D(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleCreateComposite(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleCreateAnimBlueprint(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleCompareSkeletons(const TSharedPtr<FJsonObject>& Params);

	// --- Wave 12: Sequence Properties + Sync Markers (7) ---
	static FAxonActionResult HandleSetSequenceProperties(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleSetAdditiveSettings(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleSetCompressionSettings(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleGetSyncMarkers(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleAddSyncMarker(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleRemoveSyncMarker(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleRenameSyncMarker(const TSharedPtr<FJsonObject>& Params);

	// derive_foot_sync_markers — auto-derive L/R foot-plant sync markers from data
	// already present in a clip, via a 5-signal availability cascade (first signal
	// that yields plants wins): existing markers -> footstep notifies -> contact_l/_r
	// curves -> Phase curve extrema -> component-space foot-bone speed minima. The
	// foot-speed fallback is a native port of UFootstepAnimEventsModifier's FootBoneSpeed
	// technique (GetAnimPoseAtTimeIntervals + GetBonePose World space), so the action is
	// project-agnostic and needs no per-project modifier-config assets. All thresholds,
	// bone names, marker names and notify-track patterns are overridable. Honours dry_run.
	static FAxonActionResult HandleDeriveFootSyncMarkers(const TSharedPtr<FJsonObject>& Params);

	// --- Anim-node bindings: function (Gap 2) + pin property (Gap 12) ---
	// Function bindings live on UAnimGraphNode_Base's public FMemberReference
	// UPROPERTYs (InitialUpdate/BecomeRelevant/Update Function). Pin property
	// bindings live in the node's UAnimGraphNodeBinding_Base::PropertyBindings map
	// (unlinkable class — reached via FProperty reflection). Setters mirror the
	// engine's own validate-then-recompile handshake.
	static FAxonActionResult HandleGetAnimNodeFunctionBindings(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleSetAnimNodeFunctionBinding(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleGetAnimNodePinBindings(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleSetAnimNodePinBinding(const TSharedPtr<FJsonObject>& Params);

	// --- Wave 13: Batch Ops + Montage Completion (6) ---
	static FAxonActionResult HandleBatchExecute(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleAddMontageAnimSegment(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleCloneNotifySetup(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleBulkAddNotify(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleCreateMontageFromSections(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleBuildSequenceFromPoses(const TSharedPtr<FJsonObject>& Params);
};
