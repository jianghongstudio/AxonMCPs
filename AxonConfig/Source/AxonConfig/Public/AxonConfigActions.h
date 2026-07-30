#pragma once

#include "CoreMinimal.h"
#include "AxonToolRegistry.h"

/**
 * Config/INI domain action handlers for Axon.
 * 6 read actions using GConfig/FConfigCacheIni for config hierarchy resolution,
 * plus 1 dev-gated (#if WITH_EDITOR) write action — `set_developer_setting`.
 */
class FAxonConfigActions
{
public:
	/** Register all config actions with the tool registry */
	static void RegisterActions(FAxonToolRegistry& Registry);

	// --- Action handlers ---
	static FAxonActionResult ResolveSetting(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult ExplainSetting(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult DiffFromDefault(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult SearchConfig(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult GetSection(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult GetConfigFiles(const TSharedPtr<FJsonObject>& Params);

#if WITH_EDITOR
	/**
	 * DEV-ONLY (write): set a property on a UDeveloperSettings CDO at runtime.
	 * Gated #if WITH_EDITOR — never registered in shipping/runtime builds.
	 */
	static FAxonActionResult SetDeveloperSetting(const TSharedPtr<FJsonObject>& Params);
#endif // WITH_EDITOR

private:
	/** Map shortname (e.g. "DefaultEngine") to full file path */
	static FString ResolveConfigFilePath(const FString& ShortName);

	/** Get the hierarchy of config files for a given category (e.g. "Engine", "Game") */
	static TArray<TPair<FString, FString>> GetConfigHierarchy(const FString& Category);
};
