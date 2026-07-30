#pragma once
#include "CoreMinimal.h"
#include "AxonToolRegistry.h"

class FAxonBlueprintStructActions
{
public:
	static void RegisterActions(FAxonToolRegistry& Registry);

	static FAxonActionResult HandleCreateUserDefinedStruct(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleCreateUserDefinedEnum(const TSharedPtr<FJsonObject>& Params);

	// DataTable actions (Phase 3C)
	static FAxonActionResult HandleCreateDataTable(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleAddDataTableRow(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleGetDataTableRows(const TSharedPtr<FJsonObject>& Params);

	// Raw UObject asset creation (not Blueprint)
	static FAxonActionResult HandleCreateDataAsset(const TSharedPtr<FJsonObject>& Params);

	// Create + populate a DataAsset in one call (create_data_asset body + reflection-walker fill).
	static FAxonActionResult HandleSeedDataAsset(const TSharedPtr<FJsonObject>& Params);
};
