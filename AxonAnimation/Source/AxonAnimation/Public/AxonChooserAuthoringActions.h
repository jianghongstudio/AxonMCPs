#pragma once

#include "CoreMinimal.h"
#include "AxonToolRegistry.h"

/**
 * Chooser table AUTHORING actions for Axon — registered under the existing
 * `chooser` namespace (alongside FAxonChooserActions, which owns the read /
 * remap / validate side). These three actions create a UChooserTable from
 * scratch and grow it column-by-column / row-by-row, keeping every parallel
 * per-row array aligned.
 *
 * 4 actions: create_chooser_table, add_chooser_column, add_chooser_row,
 * set_chooser_cell.
 *
 * Operates on UChooserTable assets (Chooser plugin). All handlers are gated
 * behind WITH_CHOOSER; when the Chooser plugin is absent the off-gate stub
 * returns a clean "Chooser plugin not available" error rather than failing to
 * link (mirrors AxonChooserActions.cpp:99-105).
 *
 * The mutated arrays (ResultsStructs / ColumnsStructs / DisabledRows and every
 * column's per-row value array) are WITH_EDITORONLY_DATA on UChooserTable, so
 * the bodies additionally gate on WITH_EDITORONLY_DATA and return an
 * editor-only error off-gate.
 */
class AXONANIMATION_API FAxonChooserAuthoringActions
{
public:
	/** Register all chooser-authoring actions with the tool registry. Always registers; gating is per-handler. */
	static void RegisterActions(FAxonToolRegistry& Registry);

private:
	static FAxonActionResult HandleCreateChooserTable(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleAddChooserColumn(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleAddChooserRow(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleSetChooserCell(const TSharedPtr<FJsonObject>& Params);
};
