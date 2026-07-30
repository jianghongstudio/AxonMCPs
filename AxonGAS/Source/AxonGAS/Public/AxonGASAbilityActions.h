#pragma once

#include "AxonGASInternal.h"

class FAxonGASAbilityActions
{
public:
	static void RegisterActions(FAxonToolRegistry& Registry);

private:
	// Phase 1: Core CRUD
	static FAxonActionResult HandleCreateAbility(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleGetAbilityInfo(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleListAbilities(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleCompileAbility(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleSetAbilityTags(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleGetAbilityTags(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleSetAbilityPolicy(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleSetAbilityCost(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleSetAbilityCooldown(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleSetAbilityTriggers(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleSetAbilityFlags(const TSharedPtr<FJsonObject>& Params);

	// Phase 2: Graph Building + Templates
	static FAxonActionResult HandleAddAbilityTaskNode(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleAddCommitAndEndFlow(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleAddEffectApplication(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleAddGameplayCueNode(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleCreateAbilityFromTemplate(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleBuildAbilityFromSpec(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleBatchCreateAbilities(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleDuplicateAbility(const TSharedPtr<FJsonObject>& Params);
	// Phase 2: Ability Tasks
	static FAxonActionResult HandleListAbilityTasks(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleGetAbilityTaskPins(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleWireAbilityTaskDelegate(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleGetAbilityGraphFlow(const TSharedPtr<FJsonObject>& Params);
	// Phase 3: Analysis
	static FAxonActionResult HandleValidateAbility(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleFindAbilitiesByTag(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleGetAbilityTagMatrix(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleValidateAbilityBlueprint(const TSharedPtr<FJsonObject>& Params);
	// Phase 4: Advanced
	static FAxonActionResult HandleScaffoldCustomAbilityTask(const TSharedPtr<FJsonObject>& Params);
};
