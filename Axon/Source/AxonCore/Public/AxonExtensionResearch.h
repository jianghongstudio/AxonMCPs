#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

/**
 * Domain-agnostic filesystem research for extension scaffolding.
 * Scans a target plugin's uplugin, modules, Public headers, and docs.
 * Does NOT recommend actions.
 */
class FAxonExtensionResearch
{
public:
	/**
	 * @param TargetPlugin  Plugin name (under Project/Engine Plugins) or absolute/relative path
	 * @param ExtraPaths    Optional additional dirs to scan for docs/headers
	 */
	static TSharedPtr<FJsonObject> Research(const FString& TargetPlugin, const TArray<FString>& ExtraPaths, FString& OutError);
};
