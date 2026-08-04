#pragma once

#include "AxonIndexer.h"

class FUserDefinedStructIndexer : public IAxonIndexer
{
public:
	virtual TArray<FString> GetSupportedClasses() const override
	{
		return { TEXT("UserDefinedStruct") };
	}

	virtual bool IndexAsset(const FAssetData& AssetData, UObject* LoadedAsset, FAxonIndexDatabase& DB, int64 AssetId) override;
	virtual FString GetName() const override { return TEXT("UserDefinedStructIndexer"); }
};
