// Copyright Axon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"

/**
 * Shared parameter parsing utilities for Axon action handlers.
 * Extracted from AxonMeshUtils to avoid duplication across modules.
 */
namespace AxonParamUtils
{
	/** Parse a vector from JSON params. Accepts [x,y,z] array or {"x":..,"y":..,"z":..} object. Returns false if key not found. */
	AXONCORE_API bool ParseVector(const TSharedPtr<FJsonObject>& Params, const FString& Key, FVector& Out);

	/** Parse a rotator from JSON params. Accepts [pitch,yaw,roll] array or {"pitch":..,"yaw":..,"roll":..} object. Returns false if key not found. */
	AXONCORE_API bool ParseRotator(const TSharedPtr<FJsonObject>& Params, const FString& Key, FRotator& Out);

	/** Get the current editor world from GEditor. Returns nullptr if unavailable. */
	AXONCORE_API UWorld* GetEditorWorld();

	/** Convert FVector to a JSON array [x, y, z]. */
	AXONCORE_API TArray<TSharedPtr<FJsonValue>> VectorToJsonArray(const FVector& V);

	/**
	 * Normalize a Blueprint asset path to a class path suitable for StaticLoadClass.
	 * "/Game/Foo/BP_Bar" -> "/Game/Foo/BP_Bar.BP_Bar_C"
	 * Paths already containing "." get "_C" appended if missing.
	 */
	AXONCORE_API FString NormalizeBlueprintClassPath(const FString& BlueprintPath);

	/** Parse a mobility string ("static", "stationary", "movable") into EComponentMobility. Returns false if unrecognized. */
	AXONCORE_API bool ParseMobility(const FString& MobilityStr, EComponentMobility::Type& OutMobility);
}
