#pragma once
#include "CoreMinimal.h"
#include "AxonToolRegistry.h"

/**
 * CurveTable dataset-ergonomics actions (Part B).
 *
 * First CurveTable surface in Axon — closes the read/edit/CRUD gap. All
 * actions live in the "blueprint" namespace beside the DataTable family. Each is
 * engine-generic: the table resolves by string via LoadAssetByPath/Cast.
 *
 * CurveTable rows are NOT FProperty-walkable: each row is an FRealCurve*
 * (concretely FRichCurve* or FSimpleCurve* per GetCurveTableMode()) stored in
 * UCurveTable::RowMap (TMap<FName, FRealCurve*>). So key (de)serialization is
 * bespoke, NOT a reflection-walker call.
 *
 * Mode-lock gotcha: a fresh UCurveTable is ECurveTableMode::Empty; the FIRST
 * AddRichCurve/AddSimpleCurve permanently locks rich-vs-simple. Mixed writes are
 * rejected with a clear error. Game-thread only.
 */
class FAxonBlueprintCurveTableActions
{
public:
	static void RegisterActions(FAxonToolRegistry& Registry);

	// Read keys (branches on rich/simple mode)
	static FAxonActionResult HandleReadCurveTable(const TSharedPtr<FJsonObject>& Params);

	// Write keys (replace/merge; rich-vs-simple chosen from interp_mode, mode-lock honoured)
	static FAxonActionResult HandleSetCurveTableKeys(const TSharedPtr<FJsonObject>& Params);

	// Row CRUD
	static FAxonActionResult HandleAddCurveTableRow(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleRemoveCurveTableRow(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleRenameCurveTableRow(const TSharedPtr<FJsonObject>& Params);
};
