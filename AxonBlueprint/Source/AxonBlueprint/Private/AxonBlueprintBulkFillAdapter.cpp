// SPDX-License-Identifier: MIT
// AxonBlueprintBulkFillAdapter — Phase 1 pilot adapter for the
// FAxonBulkFillRegistry. Routes "blueprint" target_namespace traffic from
// the central `bulk_fill.apply` / `describe.schema` dispatchers to the
// generic FAxonReflectionWalker against a Blueprint CDO or any UObject
// asset (DataAsset, DataTable, GameplayEffect, etc.).
//
// Pattern: mirror the existing FAxonBlueprintCDOActions::HandleSetCDOProperty
// dual-path load + engine edit cradle (transaction → Modify → PreEditChange →
// write → ReparentTransientInstancedSubobjects → FireFullCradle), but route
// the property writes through FAxonReflectionWalker::WriteTree so the
// caller can fan out arbitrarily-deep JSON trees in one shot.

#include "AxonBlueprintBulkFillAdapter.h"
#include "AxonBlueprintInternal.h"
#include "AxonBlueprintEditCradle.h"
#include "AxonBulkFillRegistry.h"
#include "AxonBulkFillTypes.h"
#include "Reflection/AxonReflectionWalker.h"
#include "AxonAssetUtils.h"
#include "AxonJsonUtils.h"
#include "UObject/UnrealType.h"
#include "Engine/Blueprint.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "ScopedTransaction.h"
#include "Dom/JsonObject.h"

namespace AxonBlueprintBulkFillInternal
{
	// Resolve the asset at TargetAsset into (TargetClass, TargetObject, OptionalBP).
	// Mirrors AxonBlueprintCDOActions.cpp:358-377 dual-path load (Blueprint CDO
	// first, then generic UObject). Returns nullptr/nullptr/nullptr on miss; caller
	// converts that into an FAxonDryRunReport with Errors=1.
	static void ResolveTarget(const FString& TargetAsset, UClass*& OutClass, UObject*& OutObject, UBlueprint*& OutBP)
	{
		OutClass = nullptr;
		OutObject = nullptr;
		OutBP = nullptr;

		// Reuse the LoadBlueprintFromParams helper by feeding it a one-key Params blob.
		// (The helper takes out-FString AssetPath so it can rewrite it; we discard the rewrite.)
		TSharedPtr<FJsonObject> SyntheticParams = MakeShared<FJsonObject>();
		SyntheticParams->SetStringField(TEXT("asset_path"), TargetAsset);
		FString ResolvedPath;
		UBlueprint* BP = AxonBlueprintInternal::LoadBlueprintFromParams(SyntheticParams, ResolvedPath);
		if (BP && BP->GeneratedClass)
		{
			OutBP = BP;
			OutClass = BP->GeneratedClass;
			OutObject = OutClass->GetDefaultObject(false);
			return;
		}

		// Not a Blueprint — try as a generic UObject (DataAsset, GameplayEffect, etc.)
		if (UObject* Asset = FAxonAssetUtils::LoadAssetByPath(TargetAsset))
		{
			OutObject = Asset;
			OutClass = Asset->GetClass();
		}
	}

	// Build a single-error FAxonDryRunReport for use when target resolution fails.
	// Keeps the dispatcher's return shape uniform whether the walker ran or not.
	static FAxonDryRunReport MakeResolveFailureReport(const FString& Reason)
	{
		FAxonDryRunReport Report;
		FAxonBulkFillFieldWrite Write;
		Write.Path = TEXT("(adapter)");
		Write.bOk = false;
		Write.Reason = Reason;
		Report.FieldWrites.Add(Write);
		Report.Errors = 1;
		Report.bWouldApply = false;
		return Report;
	}
}

// ---------------------------------------------------------------------------
// FBulkFillAdapter — invoked from bulk_fill.apply for target_namespace="blueprint"
// ---------------------------------------------------------------------------
FAxonDryRunReport FAxonBlueprintBulkFillAdapter::BlueprintBulkFill(const FAxonBulkFillSpec& Spec)
{
	using namespace AxonBlueprintBulkFillInternal;

	if (!Spec.Tree.IsValid())
	{
		return MakeResolveFailureReport(TEXT("blueprint adapter: spec.tree is null"));
	}

	UClass* TargetClass = nullptr;
	UObject* TargetObject = nullptr;
	UBlueprint* BP = nullptr;
	ResolveTarget(Spec.TargetAsset, TargetClass, TargetObject, BP);

	if (!TargetObject || !TargetClass)
	{
		return MakeResolveFailureReport(FString::Printf(
			TEXT("blueprint adapter: asset not found or has no class: %s"), *Spec.TargetAsset));
	}

	// Dry-run: walk the tree WITHOUT mutation.
	if (Spec.bDryRun)
	{
		FAxonDryRunReport Report = FAxonReflectionWalker::InspectTree(
			Spec.Tree, TargetClass, TargetObject, Spec);
		// InspectTree should set bWouldApply=false explicitly; mirror here for safety.
		Report.bWouldApply = false;
		return Report;
	}

	// --- Engine edit cradle (mirror AxonBlueprintCDOActions.cpp:417-424).
	// Cross-package TObjectPtr refs would otherwise be silently dropped by
	// FLinkerSave's harvest walk on the next save (#29).
	TargetObject->SetFlags(RF_Transactional);
	FScopedTransaction Transaction(NSLOCTEXT(
		"AxonBlueprintBulkFillAdapter", "BlueprintBulkFill",
		"Axon Blueprint Bulk Fill"));
	TargetObject->Modify();

	// PreEditChange the whole object — walker writes into N properties, so we
	// notify at the container level (matches the Details panel's "edit multiple
	// fields" flow). PostEditChangeChainProperty will fire per-field inside
	// FireFullCradle below.
	TargetObject->PreEditChange(nullptr);

	// --- Walk + write.
	FAxonDryRunReport Report = FAxonReflectionWalker::WriteTree(
		Spec.Tree, TargetClass, TargetObject, TargetObject, Spec);

	// If any field rejected and strict was on, cancel the transaction so the
	// partial-write doesn't pollute the undo buffer. Strict-mode contract
	// (FAxonDryRunReport::bWouldApply false) is the walker's responsibility.
	if (!Report.bWouldApply)
	{
		Transaction.Cancel();
		return Report;
	}

	// --- Post-write cradle (mirror AxonBlueprintCDOActions.cpp:480 + 485).
	// FJsonObjectConverter outers new subobjects to /Engine/Transient when its
	// container isn't a UObject (JsonObjectConverter.cpp:964); FLinkerSave drops
	// those refs at save. Reparent BEFORE FireFullCradle so the cradle's Pre/Post
	// fires on correct outers.
	// We pass nullptr for the FProperty since WriteTree may touch many fields;
	// the helper iterates the whole TargetObject when Prop is null... actually
	// looking at AxonBlueprintEditCradle.h:30, ReparentTransientInstancedSubobjects
	// takes a specific FProperty. For bulk_fill we iterate the top-level fields
	// the tree touched and fire per-prop.
	for (const auto& KV : Spec.Tree->Values)
	{
		FProperty* TopProp = FAxonReflectionWalker::FindPropertyForwarding(TargetClass, AxonKeyToString(KV.Key));
		if (TopProp)
		{
			AxonEditCradle::ReparentTransientInstancedSubobjects(TargetObject, TopProp);
			AxonEditCradle::FireFullCradle(TargetObject, TopProp);
		}
	}

	// --- Mark dirty (PostEditChangeChainProperty handles property-change
	//     notifications; MarkBlueprintAsModified handles BP-asset-level recompile
	//     bookkeeping). Same split as AxonBlueprintCDOActions.cpp:493-500.
	if (BP)
	{
		FBlueprintEditorUtils::MarkBlueprintAsModified(BP);
	}
	else
	{
		TargetObject->MarkPackageDirty();
	}

	Report.bWouldApply = true;
	return Report;
}

// ---------------------------------------------------------------------------
// FDescribeAdapter — invoked from describe.schema for target_namespace="blueprint"
// ---------------------------------------------------------------------------
FAxonSchemaDescriptor FAxonBlueprintBulkFillAdapter::BlueprintDescribe(const FString& TargetAsset)
{
	using namespace AxonBlueprintBulkFillInternal;

	UClass* TargetClass = nullptr;
	UObject* TargetObject = nullptr;
	UBlueprint* BP = nullptr;
	ResolveTarget(TargetAsset, TargetClass, TargetObject, BP);

	if (!TargetClass)
	{
		FAxonSchemaDescriptor Empty;
		Empty.FieldPath = TEXT("(adapter)");
		Empty.TypeName = TEXT("error");
		Empty.ImportTextForm = FString::Printf(
			TEXT("blueprint describe: asset not found or has no class: %s"), *TargetAsset);
		return Empty;
	}

	// The walker emits the full FAxonSchemaDescriptor tree. Per design B.* cross-cutting
	// quirk table, future engine-quirk gating (bSetOnce, bPieBlocked, ConditionalOn)
	// would be applied here — Phase 1 ships the unaugmented tree; Phases 2-5 layer
	// per-namespace gating in their own adapters.
	return FAxonReflectionWalker::DescribeStruct(TargetClass);
}

// ---------------------------------------------------------------------------
// Registration entry-points (called from FAxonBlueprintModule::StartupModule
// / ShutdownModule).
// ---------------------------------------------------------------------------
void FAxonBlueprintBulkFillAdapter::Register()
{
	// FBulkFillAdapter / FDescribeAdapter are TFunction<>; pass function pointers
	// directly (TFunction has a converting constructor for compatible callables).
	// CreateStatic is delegate-class API and would not compile here — that was
	// a plan-snippet shorthand at plans/2026-05-11-axon-mcp-ergonomics.md:1139.
	FAxonBulkFillRegistry::Get().RegisterAdapter(
		TEXT("blueprint"),
		FAxonBulkFillRegistry::FBulkFillAdapter(&FAxonBlueprintBulkFillAdapter::BlueprintBulkFill),
		FAxonBulkFillRegistry::FDescribeAdapter(&FAxonBlueprintBulkFillAdapter::BlueprintDescribe));
}

void FAxonBlueprintBulkFillAdapter::Unregister()
{
	FAxonBulkFillRegistry::Get().UnregisterAdapter(TEXT("blueprint"));
}
