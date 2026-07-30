#pragma once
#include "CoreMinimal.h"
#include "AxonToolRegistry.h"

class FAxonBlueprintTemplateActions
{
public:
	static void RegisterActions(FAxonToolRegistry& Registry);

	static FAxonActionResult HandleApplyTemplate(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleListTemplates(const TSharedPtr<FJsonObject>& Params);
};
