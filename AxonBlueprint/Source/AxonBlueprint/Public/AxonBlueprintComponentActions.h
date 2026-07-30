#pragma once
#include "CoreMinimal.h"
#include "AxonToolRegistry.h"

class FAxonBlueprintComponentActions
{
public:
	static void RegisterActions(FAxonToolRegistry& Registry);

	static FAxonActionResult HandleAddComponent(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleRemoveComponent(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleRenameComponent(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleReparentComponent(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleSetComponentProperty(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleDuplicateComponent(const TSharedPtr<FJsonObject>& Params);
};
