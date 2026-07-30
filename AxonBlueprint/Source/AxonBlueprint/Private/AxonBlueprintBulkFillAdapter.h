// SPDX-License-Identifier: MIT
// Private declaration for Phase 1 blueprint pilot adapter.
// Pattern: mirrors Phase 0 Plugins/Axon/Source/AxonCore/Private/Actions/AxonBulkFillActions.h.

#pragma once

#include "CoreMinimal.h"

struct FAxonBulkFillSpec;
struct FAxonDryRunReport;
struct FAxonSchemaDescriptor;

/**
 * Phase 1 pilot — bulk_fill / describe adapter for target_namespace="blueprint".
 * Self-registers with FAxonBulkFillRegistry from FAxonBlueprintModule::StartupModule.
 *
 * Targets Blueprint CDOs (BP->GeneratedClass->GetDefaultObject(false)) AND generic
 * UObject assets (DataAsset, DataTable, GameplayEffect, AbilitySet, InputAction, etc.) —
 * the same dual-path the existing FAxonBlueprintCDOActions::HandleSetCDOProperty
 * supports.
 */
class FAxonBlueprintBulkFillAdapter
{
public:
	/** Register the adapter pair with FAxonBulkFillRegistry under namespace "blueprint". */
	static void Register();

	/** Unregister (called from FAxonBlueprintModule::ShutdownModule). */
	static void Unregister();

	/** Internal: bulk_fill.apply handler for target_namespace="blueprint". */
	static FAxonDryRunReport BlueprintBulkFill(const FAxonBulkFillSpec& Spec);

	/** Internal: describe.schema handler for target_namespace="blueprint". */
	static FAxonSchemaDescriptor BlueprintDescribe(const FString& TargetAsset);
};
