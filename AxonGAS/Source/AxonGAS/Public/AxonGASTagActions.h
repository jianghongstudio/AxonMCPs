#pragma once

#include "AxonGASInternal.h"

class FAxonGASTagActions
{
public:
	static void RegisterActions(FAxonToolRegistry& Registry);

private:
	// Phase 1: Tag CRUD
	static FAxonActionResult HandleAddGameplayTags(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleGetTagHierarchy(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleSearchTagUsage(const TSharedPtr<FJsonObject>& Params);

	// Phase 2: Productivity
	static FAxonActionResult HandleScaffoldTagHierarchy(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleRenameTag(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleRemoveGameplayTags(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleValidateTagConsistency(const TSharedPtr<FJsonObject>& Params);
	// Phase 3: Advanced
	static FAxonActionResult HandleAuditTagNaming(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleExportTagHierarchy(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleImportTagHierarchy(const TSharedPtr<FJsonObject>& Params);
};
