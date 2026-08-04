#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "AxonToolRegistry.h"

/** Helpers that invoke other Axon actions and write JSON under Knowledge/_raw/. */
class AXONKNOWLEDGELIB_API FAxonKnowledgeExtract
{
public:
	static FString SanitizeAssetFileStem(const FString& AssetPath);

	static bool WriteJsonToRaw(
		const FString& PluginName,
		const FString& RelativePathUnderRaw,
		const TSharedPtr<FJsonObject>& Json,
		FString& OutAbsPath,
		FString& OutError);

	static FAxonActionResult InvokeAndWrite(
		const FString& PluginName,
		const FString& TargetNamespace,
		const FString& TargetAction,
		const TSharedPtr<FJsonObject>& Params,
		const FString& RelativePathUnderRaw);

	static FAxonActionResult HandleExtractWrite(const FString& PluginName, const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleExtractInvoke(const FString& PluginName, const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleExtractStateMachines(const FString& PluginName, const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleExtractAnimGraphOverview(const FString& PluginName, const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleExtractChooser(const FString& PluginName, const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleExtractConfigDdcvars(const FString& PluginName, const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleExtractBundle(const FString& PluginName, const TSharedPtr<FJsonObject>& Params);
};
