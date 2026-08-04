#include "Actions/ProjectSearchGameplayTagsAction.h"
#include "AxonIndexSubsystem.h"
#include "AxonParamSchema.h"
#include "SQLiteDatabase.h"
#include "Editor.h"

FAxonActionResult FProjectSearchGameplayTagsAction::Execute(const TSharedPtr<FJsonObject>& Params)
{
	FString Query = Params->GetStringField(TEXT("query"));
	if (Query.IsEmpty())
	{
		return FAxonActionResult::Error(TEXT("'query' parameter is required"), -32602);
	}

	UAxonIndexSubsystem* Subsystem = GEditor->GetEditorSubsystem<UAxonIndexSubsystem>();
	if (!Subsystem)
	{
		return FAxonActionResult::Error(TEXT("Index subsystem not available"));
	}

	FAxonIndexDatabase* DB = Subsystem->GetDatabase();
	if (!DB || !DB->IsOpen())
	{
		return FAxonActionResult::Error(TEXT("Project index database not available"));
	}

	FSQLiteDatabase* RawDB = DB->GetRawDatabase();
	if (!RawDB)
	{
		return FAxonActionResult::Error(TEXT("Raw database handle not available"));
	}

	// Query tags with aggregated referencing asset paths
	FSQLitePreparedStatement Stmt;
	Stmt.Create(*RawDB, TEXT(
		"SELECT t.id, t.tag_name, t.parent_tag, t.reference_count, "
		"GROUP_CONCAT(a.package_path) as referencing_assets "
		"FROM tags t "
		"LEFT JOIN tag_references tr ON t.id = tr.tag_id "
		"LEFT JOIN assets a ON tr.asset_id = a.id "
		"WHERE t.tag_name LIKE ? "
		"GROUP BY t.id "
		"ORDER BY t.reference_count DESC, t.tag_name;"
	));

	FString LikePattern = TEXT("%") + Query + TEXT("%");
	Stmt.SetBindingValueByIndex(1, LikePattern);

	TArray<TSharedPtr<FJsonValue>> TagsArr;
	while (Stmt.Step() == ESQLitePreparedStatementStepResult::Row)
	{
		int64 TagId = 0;
		FString TagName, ParentTag, ReferencingAssetsRaw;
		int64 RefCount = 0;

		Stmt.GetColumnValueByIndex(0, TagId);
		Stmt.GetColumnValueByIndex(1, TagName);
		Stmt.GetColumnValueByIndex(2, ParentTag);
		Stmt.GetColumnValueByIndex(3, RefCount);
		Stmt.GetColumnValueByIndex(4, ReferencingAssetsRaw);

		auto Entry = MakeShared<FJsonObject>();
		Entry->SetStringField(TEXT("tag_name"), TagName);
		Entry->SetStringField(TEXT("parent_tag"), ParentTag);
		Entry->SetNumberField(TEXT("reference_count"), static_cast<double>(RefCount));

		// Parse comma-separated asset paths into array
		TArray<TSharedPtr<FJsonValue>> AssetsArr;
		if (!ReferencingAssetsRaw.IsEmpty())
		{
			TArray<FString> AssetPaths;
			ReferencingAssetsRaw.ParseIntoArray(AssetPaths, TEXT(","));
			for (const FString& Path : AssetPaths)
			{
				FString Trimmed = Path.TrimStartAndEnd();
				if (!Trimmed.IsEmpty())
				{
					AssetsArr.Add(MakeShared<FJsonValueString>(Trimmed));
				}
			}
		}
		Entry->SetArrayField(TEXT("referencing_assets"), AssetsArr);

		TagsArr.Add(MakeShared<FJsonValueObject>(Entry));
	}

	auto Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);
	Result->SetArrayField(TEXT("tags"), TagsArr);
	Result->SetNumberField(TEXT("count"), TagsArr.Num());
	Result->SetStringField(TEXT("query"), Query);
	return FAxonActionResult::Success(Result);
}

TSharedPtr<FJsonObject> FProjectSearchGameplayTagsAction::GetSchema()
{
	return FParamSchemaBuilder()
		.Required(TEXT("query"), TEXT("string"), TEXT("Substring to search for in tag names (e.g. 'Damage', 'Weapon')"))
		.Build();
}
