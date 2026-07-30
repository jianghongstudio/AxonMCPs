// Copyright Axon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AxonToolRegistry.h"

class FAxonBlueprintSpawnActions
{
public:
	static void RegisterActions(FAxonToolRegistry& Registry);

	static FAxonActionResult HandleSpawnBlueprintActor(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleBatchSpawnBlueprintActors(const TSharedPtr<FJsonObject>& Params);
};
