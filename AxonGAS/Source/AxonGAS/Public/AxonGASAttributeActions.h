#pragma once

#include "AxonGASInternal.h"

class FAxonGASAttributeActions
{
public:
	static void RegisterActions(FAxonToolRegistry& Registry);

private:
	// Phase 1: Core CRUD
	static FAxonActionResult HandleCreateAttributeSet(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleAddAttribute(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleGetAttributeSet(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleSetAttributeDefaults(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleListAttributeSets(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleConfigureAttributeClamping(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleConfigureMetaAttributes(const TSharedPtr<FJsonObject>& Params);

	// Phase 2: Productivity
	static FAxonActionResult HandleCreateAttributeSetFromTemplate(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleCreateAttributeInitDataTable(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleDuplicateAttributeSet(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleConfigureAttributeReplication(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleLinkDataTableToASC(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleBulkEditAttributes(const TSharedPtr<FJsonObject>& Params);
	// Phase 3: Analysis
	static FAxonActionResult HandleValidateAttributeSet(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleFindAttributeModifiers(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleDiffAttributeSets(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleGetAttributeDependencyGraph(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleRemoveAttribute(const TSharedPtr<FJsonObject>& Params);
	// Phase 4: Runtime
	static FAxonActionResult HandleGetAttributeValue(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleSetAttributeValue(const TSharedPtr<FJsonObject>& Params);
};
