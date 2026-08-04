#pragma once

#include "CoreMinimal.h"
#include "AxonIndexDatabase.h"

class IAssetRegistry;
struct FAssetData;

/**
 * Base interface for all asset indexers.
 * Each indexer knows how to deeply inspect one or more asset types
 * and write structured data into the index database.
 */
class AXONINDEX_API IAxonIndexer
{
public:
	virtual ~IAxonIndexer() = default;

	/** Return the asset classes this indexer handles (e.g. "Blueprint", "Material") */
	virtual TArray<FString> GetSupportedClasses() const = 0;

	/**
	 * Index a single asset. Called on a background thread.
	 * The asset is already loaded -- inspect it and write to DB.
	 * @return true if indexing succeeded
	 */
	virtual bool IndexAsset(const FAssetData& AssetData, UObject* LoadedAsset, FAxonIndexDatabase& DB, int64 AssetId) = 0;

	/** Human-readable name for logging */
	virtual FString GetName() const = 0;

	/** Whether this is a sentinel indexer (does its own AR enumeration) */
	virtual bool IsSentinel() const { return false; }

	/** Whether this sentinel supports scoped (incremental) indexing */
	virtual bool SupportsIncrementalIndex() const { return false; }

	/** Re-index only specified assets. Called AFTER main transaction commits — may open own transaction. */
	virtual bool IndexScoped(const TSet<FString>& ChangedPaths, const TSet<FString>& RemovedPaths, FAxonIndexDatabase& DB) { return false; }
};
