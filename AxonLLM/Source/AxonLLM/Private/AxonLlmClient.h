#pragma once

#include "CoreMinimal.h"
#include "AxonLLMSettings.h"

struct FAxonLlmTagsResult
{
	bool bOk = false;
	FString Error;
	TArray<FString> Models;
	double LatencyMs = 0.0;
};

struct FAxonLlmChatResult
{
	bool bOk = false;
	FString Error;
	FString Content;
	int32 CharsIn = 0;
	int32 CharsOut = 0;
	double LatencyMs = 0.0;
};

DECLARE_DELEGATE_OneParam(FAxonLlmChatComplete, FAxonLlmChatResult);

class FAxonLlmClient
{
public:
	/** Sync (ticks HTTP until done). Safe on game thread; OK for short UI fetches. */
	static FAxonLlmTagsResult GetTags(const FString& BaseUrl, int32 TimeoutSec);

	/** Sync chat — blocks calling thread via HttpManager.Tick loop. */
	static FAxonLlmChatResult Chat(
		const FAxonLlmWorkerProfile& Worker,
		const FString& SystemPrompt,
		const FString& UserPrompt);

	/** Non-blocking chat; OnComplete runs on the HTTP completion thread (usually game thread). */
	static void ChatAsync(
		const FAxonLlmWorkerProfile& Worker,
		const FString& SystemPrompt,
		const FString& UserPrompt,
		FAxonLlmChatComplete OnComplete);

	static FString StripThinking(const FString& Text);

private:
	static bool HttpExchange(
		const FString& Verb,
		const FString& Url,
		const FString& ContentType,
		const FString& Body,
		int32 TimeoutSec,
		int32& OutHttpCode,
		FString& OutBody,
		FString& OutError,
		double& OutLatencyMs);

	static FString BuildChatBody(const FAxonLlmWorkerProfile& Worker, const FString& SystemPrompt, const FString& UserPrompt);
	static FAxonLlmChatResult ParseChatResponse(const FString& ResponseBody, int32 CharsIn, double LatencyMs);
};
