#pragma once
#include "CoreMinimal.h"
#include "AxonToolRegistry.h"

/**
 * DataTable dataset-ergonomics actions (Part B).
 *
 * All actions live in the "blueprint" namespace beside the existing
 * create_data_table / add_data_table_row / get_data_table_rows family. Each is
 * engine-generic: row structs resolve by string, and schema/type handling is
 * delegated to FAxonReflectionWalker (AxonCore) so this file never
 * reinvents reflection. Game-thread only.
 */
class FAxonBlueprintDataTableActions
{
public:
	static void RegisterActions(FAxonToolRegistry& Registry);

	// Read + schema
	static FAxonActionResult HandleReadDataTable(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleDescribeDataTableSchema(const TSharedPtr<FJsonObject>& Params);

	// Bulk upsert/add/update (dry_run + strict, FAxonDryRunReport-shaped per field)
	static FAxonActionResult HandleSetDataTableRows(const TSharedPtr<FJsonObject>& Params);

	// Row CRUD
	static FAxonActionResult HandleRemoveDataTableRow(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleRenameDataTableRow(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleDuplicateDataTableRow(const TSharedPtr<FJsonObject>& Params);

	// Whole-table JSON/CSV round-trip
	static FAxonActionResult HandleExportDataTable(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleImportDataTable(const TSharedPtr<FJsonObject>& Params);
};
