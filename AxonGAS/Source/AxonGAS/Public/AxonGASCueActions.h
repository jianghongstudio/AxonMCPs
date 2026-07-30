#pragma once

#include "AxonGASInternal.h"

class FAxonGASCueActions
{
public:
	static void RegisterActions(FAxonToolRegistry& Registry);

private:
	// Phase 2: Cue CRUD
	static FAxonActionResult HandleCreateGameplayCueNotify(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleLinkCueToEffect(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleUnlinkCueFromEffect(const TSharedPtr<FJsonObject>& Params);
	// Phase 3: Cue Productivity
	static FAxonActionResult HandleGetCueInfo(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleListGameplayCues(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleSetCueParameters(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleFindCueTriggers(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleValidateCueCoverage(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleBatchCreateCues(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleScaffoldCueLibrary(const TSharedPtr<FJsonObject>& Params);
};
