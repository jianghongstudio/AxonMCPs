#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "AxonSettings.generated.h"

UENUM()
enum class EAxonLogVerbosity : uint8
{
	Quiet,
	Normal,
	Verbose,
	VeryVerbose
};

UCLASS(config=Axon, defaultconfig, meta=(DisplayName="Axon"))
class AXONCORE_API UAxonSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UAxonSettings();

	/** Master enable for the Axon MCP HTTP server. Takes effect on next editor restart. */
	UPROPERTY(config, EditAnywhere, Category="MCP Server")
	bool bMcpServerEnabled = true;

	/** Port for the embedded MCP HTTP server */
	UPROPERTY(config, EditAnywhere, Category="MCP Server", meta=(ClampMin="1024", ClampMax="65535"))
	int32 ServerPort = 9320;

	// --- Indexing ---

	/** Content paths to index in addition to /Game. Add plugin mount points like /MyPlugin. */
	UPROPERTY(config, EditAnywhere, Category="Indexing")
	TArray<FString> AdditionalContentPaths;

	/** Override path for ProjectIndex.db (empty = default Saved/ location) */
	UPROPERTY(config, EditAnywhere, Category="Indexing", meta=(RelativePath))
	FDirectoryPath DatabasePathOverride;

	/** Override path for engine source DB (empty = default Saved/ location) */
	UPROPERTY(config, EditAnywhere, Category="Indexing", meta=(RelativePath))
	FDirectoryPath EngineSourceDBPathOverride;

	/** Path to UE Engine/Source directory (empty = auto-detect) */
	UPROPERTY(config, EditAnywhere, Category="Indexing", meta=(RelativePath))
	FDirectoryPath EngineSourcePath;

	// --- Indexer Toggles ---

	UPROPERTY(config, EditAnywhere, Category="Indexing|Deep Indexers")
	bool bIndexBlueprints = true;

	UPROPERTY(config, EditAnywhere, Category="Indexing|Deep Indexers")
	bool bIndexMaterials = true;

	UPROPERTY(config, EditAnywhere, Category="Indexing|Deep Indexers")
	bool bIndexGenericAssets = true;

	UPROPERTY(config, EditAnywhere, Category="Indexing|Deep Indexers")
	bool bIndexNiagara = true;

	UPROPERTY(config, EditAnywhere, Category="Indexing|Deep Indexers")
	bool bIndexUserDefinedEnums = true;

	UPROPERTY(config, EditAnywhere, Category="Indexing|Deep Indexers")
	bool bIndexUserDefinedStructs = true;

	UPROPERTY(config, EditAnywhere, Category="Indexing|Deep Indexers")
	bool bIndexInputActions = true;

	UPROPERTY(config, EditAnywhere, Category="Indexing|Deep Indexers")
	bool bIndexDataAssets = true;

	UPROPERTY(config, EditAnywhere, Category="Indexing|Deep Indexers")
	bool bIndexGAS = true;

	UPROPERTY(config, EditAnywhere, Category="Indexing|Deep Indexers", meta=(DisplayName="Index MetaSounds"))
	bool bIndexMetaSounds = true;

	UPROPERTY(config, EditAnywhere, Category="Indexing|Deep Indexers")
	bool bIndexAI = true;

	UPROPERTY(config, EditAnywhere, Category="Indexing|Deep Indexers")
	bool bIndexLevelSequences = true;

	UPROPERTY(config, EditAnywhere, Category="Indexing|Post-Pass Indexers")
	bool bIndexDependencies = true;

	UPROPERTY(config, EditAnywhere, Category="Indexing|Post-Pass Indexers")
	bool bIndexLevels = true;

	UPROPERTY(config, EditAnywhere, Category="Indexing|Post-Pass Indexers")
	bool bIndexDataTables = true;

	UPROPERTY(config, EditAnywhere, Category="Indexing|Post-Pass Indexers")
	bool bIndexConfigs = true;

	UPROPERTY(config, EditAnywhere, Category="Indexing|Post-Pass Indexers")
	bool bIndexCppSymbols = true;

	UPROPERTY(config, EditAnywhere, Category="Indexing|Post-Pass Indexers")
	bool bIndexAnimations = true;

	UPROPERTY(config, EditAnywhere, Category="Indexing|Post-Pass Indexers")
	bool bIndexGameplayTags = true;

	UPROPERTY(config, EditAnywhere, Category="Indexing|Post-Pass Indexers")
	bool bIndexMeshCatalog = true;

	UPROPERTY(config, EditAnywhere, Category="Indexing")
	bool bIndexMarketplacePlugins = true;

	// --- Indexing Performance ---

	UPROPERTY(config, EditAnywhere, Category="Indexing|Performance", DisplayName="Memory Budget (MB)",
		meta=(ClampMin="0", ClampMax="65536"))
	int32 MemoryBudgetMB = 0;

	UPROPERTY(config, EditAnywhere, Category="Indexing|Performance", DisplayName="Deep Index Batch Size",
		meta=(ClampMin="0", ClampMax="64"))
	int32 DeepIndexBatchSize = 0;

	UPROPERTY(config, EditAnywhere, Category="Indexing|Performance", DisplayName="Post-Pass Batch Size",
		meta=(ClampMin="0", ClampMax="32"))
	int32 PostPassBatchSize = 0;

	UPROPERTY(config, EditAnywhere, Category="Indexing|Performance", DisplayName="GC Frequency (Batches)",
		meta=(ClampMin="1", ClampMax="20"))
	int32 GCFrequencyBatches = 2;

	UPROPERTY(config, EditAnywhere, Category="Indexing|Performance", DisplayName="Yield Time (seconds)",
		meta=(ClampMin="0.0", ClampMax="1.0"))
	float YieldTimeSeconds = 0.1f;

	UPROPERTY(config, EditAnywhere, Category="Indexing|Performance", DisplayName="Defer First-Time Index")
	bool bDeferFirstTimeIndex = false;

	UPROPERTY(config, EditAnywhere, Category="Indexing|Performance", DisplayName="Log Memory Stats")
	bool bLogMemoryStats = false;

	// --- Module Toggles ---

	UPROPERTY(config, EditAnywhere, Category="Modules")
	bool bEnableIndex = true;

	UPROPERTY(config, EditAnywhere, Category="Modules")
	bool bEnableSource = true;

	/** Log verbosity for Axon systems */
	UPROPERTY(config, EditAnywhere, Category="Logging")
	EAxonLogVerbosity LogVerbosity = EAxonLogVerbosity::Normal;

	static const UAxonSettings* Get();

	/** Returns /Game plus all AdditionalContentPaths as FName array for FARFilter usage */
	static TArray<FName> GetIndexedContentPaths();

	/** Returns true if the given package path starts with any indexed content path */
	static bool IsIndexedContentPath(const FString& PackagePath);

	virtual FName GetCategoryName() const override { return FName(TEXT("Plugins")); }
};
