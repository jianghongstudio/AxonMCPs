#pragma once

#include "CoreMinimal.h"
#include "AxonToolRegistry.h"

class FAxonBlueprintActions
{
public:
	static void RegisterActions();

	static FAxonActionResult HandleListGraphs(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleGetGraphData(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleGetGraphSummary(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleGetVariables(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleGetExecutionFlow(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleSearchNodes(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleGetComponents(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleGetComponentDetails(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleGetFunctions(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleGetEventDispatchers(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleGetParentClass(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleGetInterfaces(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleGetConstructionScript(const TSharedPtr<FJsonObject>& Params);

	// Wave 3 — Discovery & Resolution
	static FAxonActionResult HandleSearchFunctions(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleGetNodeDetails(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleGetInterfaceFunctions(const TSharedPtr<FJsonObject>& Params);

	// Wave 6 — Inspection & Editing
	static FAxonActionResult HandleGetFunctionSignature(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleGetEventDispatcherDetails(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleGetBlueprintInfo(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleFindVariableReferences(const TSharedPtr<FJsonObject>& Params);

private:
	static UBlueprint* LoadBlueprint(const TSharedPtr<FJsonObject>& Params, FString& OutAssetPath);
};
