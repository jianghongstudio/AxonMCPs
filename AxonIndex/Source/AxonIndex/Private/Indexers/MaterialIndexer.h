#pragma once

#include "AxonIndexer.h"

/**
 * Indexes Materials and Material Instances: expression nodes,
 * connections, parameters (scalar, vector, texture).
 */
class FMaterialIndexer : public IAxonIndexer
{
public:
	virtual TArray<FString> GetSupportedClasses() const override
	{
		return {
			TEXT("Material"),
			TEXT("MaterialInstanceConstant"),
			TEXT("MaterialFunction")
		};
	}

	virtual bool IndexAsset(const FAssetData& AssetData, UObject* LoadedAsset, FAxonIndexDatabase& DB, int64 AssetId) override;
	virtual FString GetName() const override { return TEXT("MaterialIndexer"); }

private:
	void IndexMaterialExpressions(class UMaterial* Material, FAxonIndexDatabase& DB, int64 AssetId);
	void IndexMaterialInstance(class UMaterialInstanceConstant* MIC, FAxonIndexDatabase& DB, int64 AssetId);
};
