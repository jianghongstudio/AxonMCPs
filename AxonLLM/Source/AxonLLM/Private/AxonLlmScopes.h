#pragma once

#include "CoreMinimal.h"
#include "AxonLLMSettings.h"
#include "AxonToolRegistry.h"
#include "Dom/JsonObject.h"

struct FAxonLlmScopeRequest
{
	EAxonLlmScope Scope = EAxonLlmScope::KnowledgeSummarizeRaw;
	FAxonLlmWorkerProfile Worker;
	int32 WorkerIndex = INDEX_NONE;
	FString KbPlugin;
	TArray<FString> Paths;
	FString Topic;
	FString ExtraInstructions;
	FString Text;
	bool bOverwrite = false;
	bool bDraftOnly = true;
	bool bDeleteDraftOnPromote = true;
	FString JobId;
};

class FAxonLlmScopes
{
public:
	static FString ResolveKnowledgeRoot(const FString& KbPlugin);
	static FString ResolveRawRoot(const FString& KbPlugin);
	static FString ResolveDraftRoot(const FString& KbPlugin);

	static bool ReadEvidenceFiles(
		const FString& KbPlugin,
		const TArray<FString>& RelativeOrAbsPaths,
		int32 MaxInputChars,
		FString& OutCombined,
		TArray<FString>& OutUsedPaths,
		bool& bOutTruncated,
		FString& OutError);

	static TArray<FString> ParsePathsParam(const TSharedPtr<FJsonObject>& Params);

	/** Build request fields from MCP params + routed worker (does not call LLM). */
	static bool BuildRequestFromParams(
		EAxonLlmScope Scope,
		const FAxonLlmWorkerProfile& Worker,
		int32 WorkerIndex,
		const TSharedPtr<FJsonObject>& Params,
		FAxonLlmScopeRequest& OutReq,
		FString& OutError);

	/** Synchronous execute (may block on HTTP for LLM scopes). */
	static FAxonActionResult ExecuteScope(const FAxonLlmScopeRequest& Req);

	/**
	 * Start async execute for LLM scopes. Returns false if sync-only scope or prep failed.
	 * OnComplete always called (success or failure).
	 */
	static bool ExecuteScopeAsync(
		const FAxonLlmScopeRequest& Req,
		TFunction<void(FAxonActionResult)> OnComplete);

	static bool ScopeNeedsLlm(EAxonLlmScope Scope);
};
