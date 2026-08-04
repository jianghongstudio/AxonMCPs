#pragma once

#include "CoreMinimal.h"
#include "AxonToolRegistry.h"

class FProjectCleanupGeneratedAssetsAction
{
public:
	static FAxonActionResult Execute(const TSharedPtr<FJsonObject>& Params);
	static FString GetName() { return TEXT("cleanup_generated_assets"); }
	static FString GetDescription() { return TEXT("Safely delete generated throwaway assets under /Game/Tests/Axon/ with reference checks and optional empty-folder removal; dry-run by default"); }
	static TSharedPtr<FJsonObject> GetSchema();
};
