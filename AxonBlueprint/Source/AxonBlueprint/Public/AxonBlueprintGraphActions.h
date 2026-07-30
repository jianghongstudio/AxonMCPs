#pragma once
#include "CoreMinimal.h"
#include "AxonToolRegistry.h"

class FAxonBlueprintGraphActions
{
public:
	static void RegisterActions(FAxonToolRegistry& Registry);

	static FAxonActionResult HandleAddFunction(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleOverrideParentFunction(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleRemoveFunction(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleRenameFunction(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleSetFunctionThreadSafe(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleAddMacro(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleRemoveMacro(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleRenameMacro(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleAddEventDispatcher(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleSetFunctionParams(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleImplementInterface(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleRemoveInterface(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleReparentBlueprint(const TSharedPtr<FJsonObject>& Params);

	// Wave 5 — Scaffolding & Templates
	static FAxonActionResult HandleScaffoldInterfaceImplementation(const TSharedPtr<FJsonObject>& Params);

	// Wave 6 — Event Dispatcher CRUD
	static FAxonActionResult HandleRemoveEventDispatcher(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleSetEventDispatcherParams(const TSharedPtr<FJsonObject>& Params);
};
