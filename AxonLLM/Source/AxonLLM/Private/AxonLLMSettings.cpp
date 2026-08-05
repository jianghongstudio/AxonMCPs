#include "AxonLLMSettings.h"
#include "AxonKnowledgeRegistry.h"

bool FAxonLlmWorkerProfile::AllowsScope(EAxonLlmScope Scope) const
{
	switch (Scope)
	{
	case EAxonLlmScope::KnowledgeSummarizeRaw:
		return bAllowSummarizeRaw;
	case EAxonLlmScope::KnowledgeDraftTopic:
		return bAllowDraftTopic;
	case EAxonLlmScope::LogSummarize:
		return bAllowLogSummarize;
	case EAxonLlmScope::KnowledgePromoteDraft:
		return bAllowPromoteDraft;
	default:
		return false;
	}
}

TArray<FString> FAxonLlmWorkerProfile::AllowedScopeWires() const
{
	TArray<FString> Out;
	if (bAllowSummarizeRaw)
	{
		Out.Add(UAxonLLMSettings::ScopeToWire(EAxonLlmScope::KnowledgeSummarizeRaw));
	}
	if (bAllowDraftTopic)
	{
		Out.Add(UAxonLLMSettings::ScopeToWire(EAxonLlmScope::KnowledgeDraftTopic));
	}
	if (bAllowLogSummarize)
	{
		Out.Add(UAxonLLMSettings::ScopeToWire(EAxonLlmScope::LogSummarize));
	}
	if (bAllowPromoteDraft)
	{
		Out.Add(UAxonLLMSettings::ScopeToWire(EAxonLlmScope::KnowledgePromoteDraft));
	}
	return Out;
}

UAxonLLMSettings::UAxonLLMSettings()
{
	CategoryName = TEXT("Plugins");

	if (Workers.Num() == 0)
	{
		FAxonLlmWorkerProfile Primary;
		Primary.BaseUrl = TEXT("http://192.168.0.42:11434");
		Primary.Model = TEXT("qwen3:14b");
		Primary.bAllowSummarizeRaw = true;
		Primary.bAllowDraftTopic = true;
		Primary.bAllowLogSummarize = true;
		Primary.bAllowPromoteDraft = true;
		Primary.TimeoutSec = 120;
		Primary.MaxInputChars = 24000;
		Primary.MaxOutputTokens = 2048;
		Primary.bEnabled = true;
		Workers.Add(Primary);

		FAxonLlmWorkerProfile Code;
		Code.BaseUrl = TEXT("http://192.168.0.42:11434");
		Code.Model = TEXT("deepseek-coder:6.7b");
		Code.bAllowSummarizeRaw = true;
		Code.bAllowDraftTopic = false;
		Code.bAllowLogSummarize = true;
		Code.bAllowPromoteDraft = false;
		Code.TimeoutSec = 120;
		Code.MaxInputChars = 24000;
		Code.MaxOutputTokens = 2048;
		Code.bEnabled = true;
		Workers.Add(Code);
	}
}

const UAxonLLMSettings* UAxonLLMSettings::Get()
{
	return GetDefault<UAxonLLMSettings>();
}

UAxonLLMSettings* UAxonLLMSettings::GetMutable()
{
	return GetMutableDefault<UAxonLLMSettings>();
}

TArray<FString> UAxonLLMSettings::DiscoverKbPluginNames()
{
	return FAxonKnowledgeRegistry::GetRegisteredPluginNames();
}

TArray<FString> UAxonLLMSettings::GetKbPluginOptions() const
{
	TArray<FString> Out;
	Out.Add(FString()); // empty = no default; Agent must pass kb_plugin
	Out.Append(DiscoverKbPluginNames());

	// Keep current value selectable even if discovery missed it.
	if (!DefaultKbPlugin.IsEmpty() && !Out.Contains(DefaultKbPlugin))
	{
		Out.Add(DefaultKbPlugin);
	}
	return Out;
}

FString UAxonLLMSettings::ScopeToWire(EAxonLlmScope Scope)
{
	switch (Scope)
	{
	case EAxonLlmScope::KnowledgeSummarizeRaw:
		return TEXT("knowledge.summarize_raw");
	case EAxonLlmScope::KnowledgeDraftTopic:
		return TEXT("knowledge.draft_topic");
	case EAxonLlmScope::LogSummarize:
		return TEXT("log.summarize");
	case EAxonLlmScope::KnowledgePromoteDraft:
		return TEXT("knowledge.promote_draft");
	default:
		return FString();
	}
}

bool UAxonLLMSettings::WireToScope(const FString& Wire, EAxonLlmScope& OutScope)
{
	if (Wire.Equals(TEXT("knowledge.summarize_raw"), ESearchCase::IgnoreCase))
	{
		OutScope = EAxonLlmScope::KnowledgeSummarizeRaw;
		return true;
	}
	if (Wire.Equals(TEXT("knowledge.draft_topic"), ESearchCase::IgnoreCase))
	{
		OutScope = EAxonLlmScope::KnowledgeDraftTopic;
		return true;
	}
	if (Wire.Equals(TEXT("log.summarize"), ESearchCase::IgnoreCase))
	{
		OutScope = EAxonLlmScope::LogSummarize;
		return true;
	}
	if (Wire.Equals(TEXT("knowledge.promote_draft"), ESearchCase::IgnoreCase))
	{
		OutScope = EAxonLlmScope::KnowledgePromoteDraft;
		return true;
	}
	return false;
}

TArray<FString> UAxonLLMSettings::AllScopeWires()
{
	return {
		ScopeToWire(EAxonLlmScope::KnowledgeSummarizeRaw),
		ScopeToWire(EAxonLlmScope::KnowledgeDraftTopic),
		ScopeToWire(EAxonLlmScope::LogSummarize),
		ScopeToWire(EAxonLlmScope::KnowledgePromoteDraft)
	};
}
