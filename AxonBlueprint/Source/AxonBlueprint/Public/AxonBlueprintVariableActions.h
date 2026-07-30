#pragma once
#include "CoreMinimal.h"
#include "AxonToolRegistry.h"

class FAxonBlueprintVariableActions
{
public:
	static void RegisterActions(FAxonToolRegistry& Registry);

	static FAxonActionResult HandleAddVariable(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleRemoveVariable(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleRenameVariable(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleSetVariableType(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleSetVariableDefaults(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleAddLocalVariable(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleRemoveLocalVariable(const TSharedPtr<FJsonObject>& Params);

	// Wave 7 — Advanced
	static FAxonActionResult HandleAddReplicatedVariable(const TSharedPtr<FJsonObject>& Params);
};
