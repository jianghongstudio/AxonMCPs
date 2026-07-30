#pragma once

#include "AxonGASInternal.h"

class FAxonGASInputActions
{
public:
	static void RegisterActions(FAxonToolRegistry& Registry);

private:
	// Phase 1: Input Binding
	static FAxonActionResult HandleSetupAbilityInputBinding(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleBindAbilityToInput(const TSharedPtr<FJsonObject>& Params);

	// Phase 2: Input Binding Productivity
	static FAxonActionResult HandleBatchBindAbilities(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleGetAbilityInputBindings(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleScaffoldInputBindingComponent(const TSharedPtr<FJsonObject>& Params);
};
