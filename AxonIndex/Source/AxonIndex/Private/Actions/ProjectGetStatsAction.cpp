#include "Actions/ProjectGetStatsAction.h"
#include "AxonIndexSubsystem.h"
#include "AxonParamSchema.h"
#include "Editor.h"

FAxonActionResult FProjectGetStatsAction::Execute(const TSharedPtr<FJsonObject>& Params)
{
	UAxonIndexSubsystem* Subsystem = GEditor->GetEditorSubsystem<UAxonIndexSubsystem>();
	if (!Subsystem)
	{
		return FAxonActionResult::Error(TEXT("Index subsystem not available"));
	}

	TSharedPtr<FJsonObject> Stats = Subsystem->GetStats();
	if (!Stats.IsValid())
	{
		return FAxonActionResult::Error(TEXT("Failed to retrieve stats"));
	}

	auto Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("success"), true);

	bool bIndexing = Subsystem->IsIndexing();
	Result->SetBoolField(TEXT("indexing"), bIndexing);
	if (bIndexing)
	{
		Result->SetNumberField(TEXT("progress"), Subsystem->GetProgress());
		Result->SetStringField(TEXT("status"), Subsystem->GetStatusMessage());
	}
	Result->SetObjectField(TEXT("stats"), Stats);
	return FAxonActionResult::Success(Result);
}

TSharedPtr<FJsonObject> FProjectGetStatsAction::GetSchema()
{
	return MakeShared<FJsonObject>();
}
