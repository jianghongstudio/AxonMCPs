#pragma once

#include "AxonGASInternal.h"

class FAxonGASASCActions
{
public:
	static void RegisterActions(FAxonToolRegistry& Registry);

private:
	// Phase 1: ASC Setup
	static FAxonActionResult HandleAddASCToActor(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleConfigureASC(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleSetupASCInit(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleSetupAbilitySystemInterface(const TSharedPtr<FJsonObject>& Params);

	// Phase 2: Configuration
	static FAxonActionResult HandleApplyASCTemplate(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleSetDefaultAbilities(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleSetDefaultEffects(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleSetDefaultAttributeSets(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleSetASCReplicationMode(const TSharedPtr<FJsonObject>& Params);
	// Phase 3: Validation
	static FAxonActionResult HandleValidateASCSetup(const TSharedPtr<FJsonObject>& Params);
	// Phase 4: Runtime
	static FAxonActionResult HandleGrantAbility(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleRevokeAbility(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleGetASCSnapshot(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleGetAllASCs(const TSharedPtr<FJsonObject>& Params);
};
