#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "AxonLLMSettings.generated.h"

/** Wire-format scope ids used by worker.run `scope`. UI uses bool checkboxes instead. */
UENUM()
enum class EAxonLlmScope : uint8
{
	KnowledgeSummarizeRaw UMETA(DisplayName = "knowledge.summarize_raw"),
	KnowledgeDraftTopic UMETA(DisplayName = "knowledge.draft_topic"),
	LogSummarize UMETA(DisplayName = "log.summarize"),
	KnowledgePromoteDraft UMETA(DisplayName = "knowledge.promote_draft"),
};

USTRUCT()
struct FAxonLlmWorkerProfile
{
	GENERATED_BODY()

	UPROPERTY(config, EditAnywhere, Category = "Worker")
	FString BaseUrl = TEXT("http://192.168.0.42:11434");

	/** Selected via dropdown after BaseUrl refresh (Ollama /api/tags). */
	UPROPERTY(config, EditAnywhere, Category = "Worker")
	FString Model = TEXT("qwen3:14b");

	UPROPERTY(config, EditAnywhere, Category = "Worker|能力", meta = (DisplayName = "摘要 _raw 证据"))
	bool bAllowSummarizeRaw = true;

	UPROPERTY(config, EditAnywhere, Category = "Worker|能力", meta = (DisplayName = "撰写知识主题草稿"))
	bool bAllowDraftTopic = true;

	UPROPERTY(config, EditAnywhere, Category = "Worker|能力", meta = (DisplayName = "摘要日志与构建错误"))
	bool bAllowLogSummarize = false;

	UPROPERTY(config, EditAnywhere, Category = "Worker|能力", meta = (DisplayName = "升格草稿为正式文档"))
	bool bAllowPromoteDraft = false;

	UPROPERTY(config, EditAnywhere, Category = "Worker", meta = (ClampMin = "5", ClampMax = "600"))
	int32 TimeoutSec = 120;

	UPROPERTY(config, EditAnywhere, Category = "Worker", meta = (ClampMin = "1024", ClampMax = "200000"))
	int32 MaxInputChars = 24000;

	UPROPERTY(config, EditAnywhere, Category = "Worker", meta = (ClampMin = "64", ClampMax = "16384"))
	int32 MaxOutputTokens = 2048;

	UPROPERTY(config, EditAnywhere, Category = "Worker")
	bool bEnabled = true;

	bool AllowsScope(EAxonLlmScope Scope) const;
	TArray<FString> AllowedScopeWires() const;
};

UCLASS(config = AxonLLM, defaultconfig, meta = (DisplayName = "Axon LLM"))
class AXONLLM_API UAxonLLMSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UAxonLLMSettings();

	UPROPERTY(config, EditAnywhere, Category = "General")
	bool bEnabled = true;

	UPROPERTY(config, EditAnywhere, Category = "General")
	bool bDraftOnly = true;

	/** After knowledge.promote_draft, delete the _draft file. */
	UPROPERTY(config, EditAnywhere, Category = "General")
	bool bDeleteDraftOnPromote = true;

	UPROPERTY(config, EditAnywhere, Category = "General")
	bool bLogUsage = true;

	/**
	 * Fallback when worker.run omits kb_plugin.
	 * Dropdown lists packs that registered via FAxonKnowledgeRegistration::RegisterAll.
	 */
	UPROPERTY(config, EditAnywhere, Category = "General",
		meta = (DisplayName = "默认知识库插件", GetOptions = "GetKbPluginOptions"))
	FString DefaultKbPlugin;

	/**
	 * Local workers. Array index is the worker id (0, 1, …).
	 * Order = priority when auto-routing a scope.
	 */
	UPROPERTY(config, EditAnywhere, Category = "Workers")
	TArray<FAxonLlmWorkerProfile> Workers;

	UFUNCTION()
	TArray<FString> GetKbPluginOptions() const;

	static const UAxonLLMSettings* Get();
	static UAxonLLMSettings* GetMutable();

	static FString ScopeToWire(EAxonLlmScope Scope);
	static bool WireToScope(const FString& Wire, EAxonLlmScope& OutScope);
	static TArray<FString> AllScopeWires();

	/** Plugin names from FAxonKnowledgeRegistry (self-registered KB packs). */
	static TArray<FString> DiscoverKbPluginNames();

	virtual FName GetCategoryName() const override { return FName(TEXT("Plugins")); }
};
