#include "Actions/ProjectListGameplayTagsAction.h"
#include "AxonIndexSubsystem.h"
#include "AxonParamSchema.h"
#include "SQLiteDatabase.h"
#include "Editor.h"

FAxonActionResult FProjectListGameplayTagsAction::Execute(const TSharedPtr<FJsonObject>& Params)
{
	FString Prefix;
	if (Params->HasField(TEXT("prefix")))
	{
		Prefix = Params->GetStringField(TEXT("prefix"));
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

	FSQLitePreparedStatement Stmt;
	if (Prefix.IsEmpty())
	{
		Stmt.Create(*RawDB, TEXT("SELECT tag_name, parent_tag, reference_count FROM tags ORDER BY tag_name;"));
	}
	else
	{
		Stmt.Create(*RawDB, TEXT("SELECT tag_name, parent_tag, reference_count FROM tags WHERE tag_name LIKE ? ORDER BY tag_name;"));
		FString LikePattern = Prefix + TEXT("%");
		Stmt.SetBindingValueByIndex(1, LikePattern);
	}

	TArray<TSharedPtr<FJsonValue>> TagsArr;
	while (Stmt.Step() == ESQLitePreparedStatementStepResult::Row)
	{
		FString TagName, ParentTag;
		int64 RefCount = 0;

		Stmt.GetColumnValueByIndex(0, TagName);
		Stmt.GetColumnValueByIndex(1, ParentTag);
		Stmt.GetColumnValueByIndex(2, RefCount);

		auto Entry = MakeShared<FJsonObject>();
		Entry->SetStringField(TEXT("tag_name"), TagName);
		Entry->SetStringField(TEXT("parent_tag"), ParentTag);
		Entry->SetNumberField(TEXT("reference_count"), static_cast<double>(RefCount));
		TagsArr.Add(MakeShared<FJsonValueObject>(Entry));
	}

	auto Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);
	Result->SetArrayField(TEXT("tags"), TagsArr);
	Result->SetNumberField(TEXT("count"), TagsArr.Num());
	if (!Prefix.IsEmpty())
	{
		Result->SetStringField(TEXT("prefix_filter"), Prefix);
	}
	return FAxonActionResult::Success(Result);
}

TSharedPtr<FJsonObject> FProjectListGameplayTagsAction::GetSchema()
{
	return FParamSchemaBuilder()
		.Optional(TEXT("prefix"), TEXT("string"), TEXT("Tag prefix filter (e.g. 'Weapon.Melee') -- returns tags starting with this prefix"))
		.Build();
}
