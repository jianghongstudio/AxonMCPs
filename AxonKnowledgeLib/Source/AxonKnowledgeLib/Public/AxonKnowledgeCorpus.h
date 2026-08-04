#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

/** In-memory Knowledge corpus for one KB plugin (Knowledge/*.md, skips _raw/). */
class AXONKNOWLEDGELIB_API FAxonKnowledgeCorpus
{
public:
	struct FDoc
	{
		FString RelPath;
		FString Title;
		FString Description;
		FString Body;
		TArray<FString> Tokens;
	};

	/** Per owning plugin name (e.g. AxonGaspKB). */
	static FAxonKnowledgeCorpus& Get(const FString& PluginName);

	explicit FAxonKnowledgeCorpus(FString InPluginName);

	FString GetPluginName() const { return PluginName; }
	FString GetKnowledgeRoot() const;
	FString GetRawRoot() const;

	void EnsureLoaded(bool bForceReload = false);

	const TArray<FDoc>& GetDocs() const { return Docs; }
	const FDoc* FindDoc(const FString& PathOrTopic) const;

	TSharedPtr<FJsonObject> ListTopicsJson() const;
	TSharedPtr<FJsonObject> SearchJson(const FString& Query, int32 MaxResults) const;
	TSharedPtr<FJsonObject> ReadJson(const FString& PathOrTopic, bool bIncludeBody) const;
	TSharedPtr<FJsonObject> RouteJson(const FString& Query) const;

private:
	void LoadInternal();
	static void Tokenize(const FString& Text, TArray<FString>& OutTokens);
	static int32 ScoreDoc(const FDoc& Doc, const TArray<FString>& QueryTokens);

	FString PluginName;
	TArray<FDoc> Docs;
	bool bLoaded = false;
};
