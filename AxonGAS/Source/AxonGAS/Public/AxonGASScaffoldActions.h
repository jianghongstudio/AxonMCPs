#pragma once

#include "AxonGASInternal.h"

class FAxonGASScaffoldActions
{
public:
	static void RegisterActions(FAxonToolRegistry& Registry);

private:
	// Phase 1: Scaffolding
	static FAxonActionResult HandleBootstrapGASFoundation(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleValidateGASSetup(const TSharedPtr<FJsonObject>& Params);

	// Phase 2: Scaffolding
	static FAxonActionResult HandleScaffoldGASProject(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleScaffoldDamagePipeline(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleScaffoldStatusEffect(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleScaffoldWeaponAbility(const TSharedPtr<FJsonObject>& Params);

	// Phase F8 (J-phase): author-time ability grant via ASC-CDO startup-array reflection
	static FAxonActionResult HandleGrantAbilityToPawn(const TSharedPtr<FJsonObject>& Params);
};
