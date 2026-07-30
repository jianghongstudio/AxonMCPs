#pragma once

#include "AxonGASInternal.h"

class FAxonGASEffectActions
{
public:
	static void RegisterActions(FAxonToolRegistry& Registry);

private:
	// Phase 1: Core CRUD
	static FAxonActionResult HandleCreateGameplayEffect(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleGetGameplayEffect(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleListGameplayEffects(const TSharedPtr<FJsonObject>& Params);
	// Modifiers
	static FAxonActionResult HandleAddModifier(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleSetModifier(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleRemoveModifier(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleListModifiers(const TSharedPtr<FJsonObject>& Params);
	// Components & Configuration
	static FAxonActionResult HandleAddGEComponent(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleSetGEComponent(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleSetEffectStacking(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleSetDuration(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleSetPeriod(const TSharedPtr<FJsonObject>& Params);

	// Phase 2: Productivity
	static FAxonActionResult HandleCreateEffectFromTemplate(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleBuildEffectFromSpec(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleBatchCreateEffects(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleAddExecution(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleDuplicateGameplayEffect(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleDeleteGameplayEffect(const TSharedPtr<FJsonObject>& Params);
	// Phase 3: Analysis
	static FAxonActionResult HandleValidateEffect(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleGetEffectInteractionMatrix(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleRemoveGEComponent(const TSharedPtr<FJsonObject>& Params);
	// Phase 4: Runtime
	static FAxonActionResult HandleGetActiveEffects(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleGetEffectModifiersBreakdown(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleApplyEffect(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleRemoveEffect(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleSimulateEffectStack(const TSharedPtr<FJsonObject>& Params);
};
