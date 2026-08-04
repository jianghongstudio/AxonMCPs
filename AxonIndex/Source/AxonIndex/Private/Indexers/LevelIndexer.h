#pragma once

#include "AxonIndexer.h"

/**
 * Indexes level actors from World/Map assets.
 * Runs after all other indexers (needs all assets in DB).
 * Uses special class name "__Levels__" for post-indexing dispatch.
 * Loads each level's persistent level to extract actor metadata.
 */
class FLevelIndexer : public IAxonIndexer
{
public:
	virtual TArray<FString> GetSupportedClasses() const override
	{
		return { TEXT("__Levels__") };
	}

	virtual bool IndexAsset(const FAssetData& AssetData, UObject* LoadedAsset, FAxonIndexDatabase& DB, int64 AssetId) override;
	virtual FString GetName() const override { return TEXT("LevelIndexer"); }
	virtual bool IsSentinel() const override { return true; }

	/** Set of valid path prefixes for indexing */
	TArray<FName> IndexedPaths;

	/** Set indexed paths. Called before IndexAsset. */
	void SetIndexedPaths(const TArray<FName>& InPaths) { IndexedPaths = InPaths; }

private:
	int32 IndexActorsInLevel(class ULevel* Level, FAxonIndexDatabase& DB, int64 AssetId);
	FString SerializeTransform(const FTransform& Transform);
	FString SerializeComponents(const class AActor* Actor);
};
