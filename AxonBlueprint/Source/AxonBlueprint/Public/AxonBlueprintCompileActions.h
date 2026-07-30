#pragma once
#include "CoreMinimal.h"
#include "AxonToolRegistry.h"

class FAxonBlueprintCompileActions
{
public:
	static void RegisterActions(FAxonToolRegistry& Registry);

	static FAxonActionResult HandleCompileBlueprint(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleValidateBlueprint(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleCreateBlueprint(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleDuplicateBlueprint(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleGetDependencies(const TSharedPtr<FJsonObject>& Params);

	// Phase 1F — save_asset
	static FAxonActionResult HandleSaveAsset(const TSharedPtr<FJsonObject>& Params);
	// Gap #10 — save all dirty Blueprint/Widget packages in one sweep
	static FAxonActionResult HandleSaveDirtyAssets(const TSharedPtr<FJsonObject>& Params);
};
