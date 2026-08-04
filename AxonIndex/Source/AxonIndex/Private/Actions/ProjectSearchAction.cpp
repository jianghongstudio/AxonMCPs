#include "Actions/ProjectSearchAction.h"
#include "AxonIndexSubsystem.h"
#include "AxonParamSchema.h"
#include "Editor.h"

FAxonActionResult FProjectSearchAction::Execute(const TSharedPtr<FJsonObject>& Params)
{
	FString Query = Params->GetStringField(TEXT("query"));
	int32 Limit = Params->HasField(TEXT("limit")) ? Params->GetIntegerField(TEXT("limit")) : 50;

	if (Query.IsEmpty())
	{
		return FAxonActionResult::Error(TEXT("'query' parameter is required"), -32602);
	}

	UAxonIndexSubsystem* Subsystem = GEditor->GetEditorSubsystem<UAxonIndexSubsystem>();
	if (!Subsystem)
	{
		return FAxonActionResult::Error(TEXT("Index subsystem not available"));
	}

	if (Subsystem->IsIndexing())
	{
		auto Result = MakeShared<FJsonObject>();
		Result->SetBoolField(TEXT("success"), false);
		Result->SetStringField(TEXT("error"), TEXT("Indexing is currently in progress"));
		Result->SetNumberField(TEXT("progress"), Subsystem->GetProgress());
		return FAxonActionResult::Success(Result);
	}

	TArray<FSearchResult> SearchResults = Subsystem->Search(Query, Limit);

	auto Result = MakeShared<FJsonObject>();
	TArray<TSharedPtr<FJsonValue>> ResultsArr;
	for (const FSearchResult& SR : SearchResults)
	{
		auto Entry = MakeShared<FJsonObject>();
		Entry->SetStringField(TEXT("asset_path"), SR.AssetPath);
		Entry->SetStringField(TEXT("asset_name"), SR.AssetName);
		Entry->SetStringField(TEXT("asset_class"), SR.AssetClass);
		Entry->SetStringField(TEXT("module_name"), SR.ModuleName);
		Entry->SetStringField(TEXT("match_context"), SR.MatchContext);
		Entry->SetNumberField(TEXT("rank"), SR.Rank);
		ResultsArr.Add(MakeShared<FJsonValueObject>(Entry));
	}

	Result->SetBoolField(TEXT("success"), true);
	Result->SetArrayField(TEXT("results"), ResultsArr);
	Result->SetNumberField(TEXT("count"), SearchResults.Num());
	return FAxonActionResult::Success(Result);
}

TSharedPtr<FJsonObject> FProjectSearchAction::GetSchema()
{
	return FParamSchemaBuilder()
		.Required(TEXT("query"), TEXT("string"), TEXT("FTS5 search query (supports AND, OR, NOT, prefix*)"))
		.Optional(TEXT("limit"), TEXT("integer"), TEXT("Maximum results to return"), TEXT("50"))
		.Build();
}
