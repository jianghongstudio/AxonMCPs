#include "AxonKnowledgeCorpus.h"

#include "Interfaces/IPluginManager.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

FAxonKnowledgeCorpus& FAxonKnowledgeCorpus::Get(const FString& PluginName)
{
	static TMap<FString, TUniquePtr<FAxonKnowledgeCorpus>> Instances;
	TUniquePtr<FAxonKnowledgeCorpus>* Found = Instances.Find(PluginName);
	if (!Found)
	{
		Found = &Instances.Add(PluginName, MakeUnique<FAxonKnowledgeCorpus>(PluginName));
	}
	return **Found;
}

FAxonKnowledgeCorpus::FAxonKnowledgeCorpus(FString InPluginName)
	: PluginName(MoveTemp(InPluginName))
{
}

FString FAxonKnowledgeCorpus::GetKnowledgeRoot() const
{
	FString Root;
	TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(PluginName);
	if (Plugin.IsValid())
	{
		Root = FPaths::Combine(Plugin->GetBaseDir(), TEXT("Knowledge"));
	}
	else
	{
		Root = FPaths::Combine(FPaths::ProjectPluginsDir(), TEXT("AxonMCPs"), PluginName, TEXT("Knowledge"));
	}
	return FPaths::ConvertRelativePathToFull(Root);
}

FString FAxonKnowledgeCorpus::GetRawRoot() const
{
	return FPaths::Combine(GetKnowledgeRoot(), TEXT("_raw"));
}

void FAxonKnowledgeCorpus::EnsureLoaded(bool bForceReload)
{
	if (bLoaded && !bForceReload)
	{
		return;
	}
	LoadInternal();
	bLoaded = true;
}

void FAxonKnowledgeCorpus::Tokenize(const FString& Text, TArray<FString>& OutTokens)
{
	OutTokens.Reset();
	FString Lower = Text.ToLower();
	FString Current;
	auto Flush = [&]()
	{
		if (Current.Len() >= 2)
		{
			OutTokens.Add(Current);
		}
		Current.Reset();
	};

	for (TCHAR Ch : Lower)
	{
		const bool bWord = FChar::IsAlnum(Ch) || Ch == TEXT('_') || Ch == TEXT('-') || Ch == TEXT('.') || Ch == TEXT('/');
		if (bWord)
		{
			Current.AppendChar(Ch);
		}
		else
		{
			Flush();
		}
	}
	Flush();
}

void FAxonKnowledgeCorpus::LoadInternal()
{
	Docs.Reset();
	const FString Root = GetKnowledgeRoot();
	if (!FPaths::DirectoryExists(Root))
	{
		UE_LOG(LogTemp, Warning, TEXT("AxonKnowledgeLib: Knowledge root missing for %s: %s"), *PluginName, *Root);
		return;
	}

	TArray<FString> Files;
	IFileManager::Get().FindFilesRecursive(Files, *Root, TEXT("*.md"), true, false);

	for (const FString& AbsPath : Files)
	{
		if (AbsPath.Contains(TEXT("/_raw/")) || AbsPath.Contains(TEXT("\\_raw\\")))
		{
			continue;
		}

		FString Body;
		if (!FFileHelper::LoadFileToString(Body, *AbsPath))
		{
			continue;
		}

		FDoc Doc;
		Doc.RelPath = AbsPath;
		FPaths::MakePathRelativeTo(Doc.RelPath, *(Root + TEXT("/")));
		Doc.RelPath.ReplaceInline(TEXT("\\"), TEXT("/"));
		Doc.Body = Body;

		Doc.Title = FPaths::GetBaseFilename(Doc.RelPath);
		TArray<FString> Lines;
		Body.ParseIntoArrayLines(Lines, false);
		FString FirstPara;
		for (const FString& Line : Lines)
		{
			const FString Trimmed = Line.TrimStartAndEnd();
			if (Trimmed.StartsWith(TEXT("# ")))
			{
				Doc.Title = Trimmed.Mid(2).TrimStartAndEnd();
				continue;
			}
			if (Trimmed.StartsWith(TEXT(">")))
			{
				if (FirstPara.IsEmpty() && Trimmed.Len() > 2)
				{
					FirstPara = Trimmed.Mid(1).TrimStartAndEnd();
				}
				continue;
			}
			if (FirstPara.IsEmpty() && !Trimmed.IsEmpty() && !Trimmed.StartsWith(TEXT("#")) && !Trimmed.StartsWith(TEXT("|")) && !Trimmed.StartsWith(TEXT("---")))
			{
				FirstPara = Trimmed;
			}
		}
		Doc.Description = FirstPara.Left(240);
		Tokenize(Doc.Title + TEXT(" ") + Doc.Description + TEXT(" ") + Doc.Body, Doc.Tokens);
		Docs.Add(MoveTemp(Doc));
	}

	Docs.Sort([](const FDoc& A, const FDoc& B) { return A.RelPath < B.RelPath; });
	UE_LOG(LogTemp, Log, TEXT("AxonKnowledgeLib: loaded %d docs for %s from %s"), Docs.Num(), *PluginName, *Root);
}

const FAxonKnowledgeCorpus::FDoc* FAxonKnowledgeCorpus::FindDoc(const FString& PathOrTopic) const
{
	if (PathOrTopic.IsEmpty())
	{
		return nullptr;
	}

	FString Needle = PathOrTopic;
	Needle.ReplaceInline(TEXT("\\"), TEXT("/"));

	for (const FDoc& Doc : Docs)
	{
		if (Doc.RelPath.Equals(Needle, ESearchCase::IgnoreCase))
		{
			return &Doc;
		}
	}
	for (const FDoc& Doc : Docs)
	{
		if (Doc.RelPath.EndsWith(Needle, ESearchCase::IgnoreCase)
			|| FPaths::GetBaseFilename(Doc.RelPath).Equals(Needle, ESearchCase::IgnoreCase)
			|| FPaths::GetBaseFilename(Doc.RelPath).Equals(FPaths::GetBaseFilename(Needle), ESearchCase::IgnoreCase))
		{
			return &Doc;
		}
	}
	return nullptr;
}

int32 FAxonKnowledgeCorpus::ScoreDoc(const FDoc& Doc, const TArray<FString>& QueryTokens)
{
	if (QueryTokens.Num() == 0)
	{
		return 0;
	}

	int32 Score = 0;
	const FString TitleLower = Doc.Title.ToLower();
	const FString PathLower = Doc.RelPath.ToLower();
	const FString DescLower = Doc.Description.ToLower();

	for (const FString& Tok : QueryTokens)
	{
		int32 Occurrences = 0;
		for (const FString& DocTok : Doc.Tokens)
		{
			if (DocTok == Tok || DocTok.Contains(Tok))
			{
				++Occurrences;
			}
		}
		Score += Occurrences;
		if (TitleLower.Contains(Tok))
		{
			Score += 8;
		}
		if (PathLower.Contains(Tok))
		{
			Score += 5;
		}
		if (DescLower.Contains(Tok))
		{
			Score += 3;
		}
	}
	return Score;
}

TSharedPtr<FJsonObject> FAxonKnowledgeCorpus::ListTopicsJson() const
{
	TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();
	TArray<TSharedPtr<FJsonValue>> Arr;
	for (const FDoc& Doc : Docs)
	{
		TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
		Obj->SetStringField(TEXT("path"), Doc.RelPath);
		Obj->SetStringField(TEXT("title"), Doc.Title);
		Obj->SetStringField(TEXT("description"), Doc.Description);
		Obj->SetNumberField(TEXT("chars"), Doc.Body.Len());
		Arr.Add(MakeShared<FJsonValueObject>(Obj));
	}
	Root->SetArrayField(TEXT("topics"), Arr);
	Root->SetNumberField(TEXT("count"), Arr.Num());
	Root->SetStringField(TEXT("knowledge_root"), GetKnowledgeRoot());
	Root->SetStringField(TEXT("plugin"), PluginName);
	return Root;
}

TSharedPtr<FJsonObject> FAxonKnowledgeCorpus::SearchJson(const FString& Query, int32 MaxResults) const
{
	TArray<FString> QueryTokens;
	Tokenize(Query, QueryTokens);

	struct FHit
	{
		const FDoc* Doc = nullptr;
		int32 Score = 0;
	};
	TArray<FHit> Hits;
	for (const FDoc& Doc : Docs)
	{
		const int32 Score = ScoreDoc(Doc, QueryTokens);
		if (Score > 0)
		{
			Hits.Add({&Doc, Score});
		}
	}
	Hits.Sort([](const FHit& A, const FHit& B) { return A.Score > B.Score; });

	const int32 Limit = FMath::Clamp(MaxResults, 1, 50);
	TArray<TSharedPtr<FJsonValue>> Arr;
	for (int32 i = 0; i < Hits.Num() && i < Limit; ++i)
	{
		const FDoc& Doc = *Hits[i].Doc;
		TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
		Obj->SetStringField(TEXT("path"), Doc.RelPath);
		Obj->SetStringField(TEXT("title"), Doc.Title);
		Obj->SetNumberField(TEXT("score"), Hits[i].Score);

		FString Snippet = Doc.Description;
		TArray<FString> Lines;
		Doc.Body.ParseIntoArrayLines(Lines, false);
		for (const FString& Line : Lines)
		{
			const FString Lower = Line.ToLower();
			bool bMatch = false;
			for (const FString& Tok : QueryTokens)
			{
				if (Lower.Contains(Tok))
				{
					bMatch = true;
					break;
				}
			}
			if (bMatch)
			{
				Snippet = Line.TrimStartAndEnd().Left(320);
				break;
			}
		}
		Obj->SetStringField(TEXT("snippet"), Snippet);
		Arr.Add(MakeShared<FJsonValueObject>(Obj));
	}

	TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetStringField(TEXT("query"), Query);
	Root->SetArrayField(TEXT("results"), Arr);
	Root->SetNumberField(TEXT("count"), Arr.Num());
	Root->SetNumberField(TEXT("scanned"), Docs.Num());
	Root->SetStringField(TEXT("plugin"), PluginName);
	return Root;
}

TSharedPtr<FJsonObject> FAxonKnowledgeCorpus::ReadJson(const FString& PathOrTopic, bool bIncludeBody) const
{
	const FDoc* Doc = FindDoc(PathOrTopic);
	TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();
	if (!Doc)
	{
		Root->SetBoolField(TEXT("found"), false);
		Root->SetStringField(TEXT("requested"), PathOrTopic);
		return Root;
	}
	Root->SetBoolField(TEXT("found"), true);
	Root->SetStringField(TEXT("path"), Doc->RelPath);
	Root->SetStringField(TEXT("title"), Doc->Title);
	Root->SetStringField(TEXT("description"), Doc->Description);
	if (bIncludeBody)
	{
		Root->SetStringField(TEXT("body"), Doc->Body);
	}
	Root->SetNumberField(TEXT("chars"), Doc->Body.Len());
	return Root;
}

TSharedPtr<FJsonObject> FAxonKnowledgeCorpus::RouteJson(const FString& Query) const
{
	TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetStringField(TEXT("query"), Query);

	TArray<TSharedPtr<FJsonValue>> Routes;
	const FDoc* Routing = FindDoc(TEXT("00-routing.md"));
	if (Routing)
	{
		TArray<FString> QueryTokens;
		Tokenize(Query, QueryTokens);
		TArray<FString> Lines;
		Routing->Body.ParseIntoArrayLines(Lines, false);
		for (const FString& Line : Lines)
		{
			if (!Line.Contains(TEXT("|")))
			{
				continue;
			}
			const FString Lower = Line.ToLower();
			bool bHit = false;
			for (const FString& Tok : QueryTokens)
			{
				if (Tok.Len() >= 3 && Lower.Contains(Tok))
				{
					bHit = true;
					break;
				}
			}
			if (bHit)
			{
				TSharedPtr<FJsonObject> Row = MakeShared<FJsonObject>();
				Row->SetStringField(TEXT("row"), Line.TrimStartAndEnd());
				Routes.Add(MakeShared<FJsonValueObject>(Row));
				if (Routes.Num() >= 12)
				{
					break;
				}
			}
		}
	}
	Root->SetArrayField(TEXT("routing_rows"), Routes);

	TSharedPtr<FJsonObject> Search = SearchJson(Query, 5);
	Root->SetArrayField(TEXT("suggested_docs"), Search->GetArrayField(TEXT("results")));
	return Root;
}
