#include "AxonPieActions.h"

#include "AxonPieObject.h"
#include "AxonPieSession.h"
#include "AxonParamSchema.h"

#include "Editor.h"
#include "Editor/UnrealEdEngine.h"
#include "UnrealEdGlobals.h"
#include "LevelEditor.h"
#include "IAssetViewport.h"
#include "LevelEditorSubsystem.h"
#include "IPythonScriptPlugin.h"
#include "PythonScriptTypes.h"
#include "Engine/Blueprint.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Modules/ModuleManager.h"
#include "CoreGlobals.h"
#include "UObject/GarbageCollection.h"
#include "UObject/UObjectIterator.h"
#include "Templates/UnrealTemplate.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"

DEFINE_LOG_CATEGORY_STATIC(LogAxonPieActions, Log, All);

namespace
{
	struct FErroredBlueprintEntry
	{
		FString Name;
		FString Path;
	};

	UWorld* FindActivePieWorld()
	{
		return AxonPieObject::FindPieWorld();
	}

	bool EnsureNoResidentPieWorldBeforeMapLoad(FString& OutError)
	{
		if (!GEditor)
		{
			OutError = TEXT("GEditor unavailable — cannot evaluate PIE residency before map load.");
			return false;
		}
		if (!FindActivePieWorld())
		{
			return true;
		}
		if (FAxonPieSessionManager::Get().HasRunningSessions())
		{
			OutError = TEXT("An Axon PIE timeseries session is still resident; stop it before loading a new map.");
			return false;
		}

		GEditor->RequestEndPlayMap();
		for (int32 Iteration = 0; Iteration < 8 && FindActivePieWorld(); ++Iteration)
		{
			GEditor->EndPlayMap();
		}
		if (FindActivePieWorld())
		{
			OutError = TEXT("PIE world teardown did not complete; refusing map load to avoid a world memory leak.");
			return false;
		}
		CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
		return true;
	}

	bool LoadMapIfRequested(const TSharedPtr<FJsonObject>& Params, FString& OutError)
	{
		FString MapPath;
		if (!Params.IsValid() || !Params->TryGetStringField(TEXT("map"), MapPath) || MapPath.IsEmpty())
		{
			return true;
		}
		if (!GEditor)
		{
			OutError = TEXT("GEditor unavailable — cannot load map for PIE.");
			return false;
		}
		if (!EnsureNoResidentPieWorldBeforeMapLoad(OutError))
		{
			return false;
		}
		ULevelEditorSubsystem* LevelEditorSubsystem = GEditor->GetEditorSubsystem<ULevelEditorSubsystem>();
		if (!LevelEditorSubsystem || !LevelEditorSubsystem->LoadLevel(MapPath))
		{
			OutError = FString::Printf(TEXT("Failed to load map '%s' for PIE."), *MapPath);
			return false;
		}
		return true;
	}

	void ScanErroredBlueprints(TArray<FErroredBlueprintEntry>& OutErrored)
	{
		for (TObjectIterator<UBlueprint> It; It; ++It)
		{
			UBlueprint* Blueprint = *It;
			if (IsValid(Blueprint) && Blueprint->Status == BS_Error && Blueprint->bDisplayCompilePIEWarning)
			{
				OutErrored.Add({ Blueprint->GetName(), Blueprint->GetPathName() });
			}
		}
	}

	TArray<TSharedPtr<FJsonValue>> ErroredBlueprintsToJson(const TArray<FErroredBlueprintEntry>& Errored)
	{
		TArray<TSharedPtr<FJsonValue>> Results;
		for (const FErroredBlueprintEntry& Entry : Errored)
		{
			TSharedPtr<FJsonObject> Item = MakeShared<FJsonObject>();
			Item->SetStringField(TEXT("name"), Entry.Name);
			Item->SetStringField(TEXT("path"), Entry.Path);
			Results.Add(MakeShared<FJsonValueObject>(Item));
		}
		return Results;
	}

	bool StartPieInternal(FString& OutError, bool bSuppressModals = false)
	{
		if (!GUnrealEd)
		{
			OutError = TEXT("GUnrealEd not available");
			return false;
		}
		if (FindActivePieWorld())
		{
			OutError = TEXT("PIE already running");
			return false;
		}

		FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>("LevelEditor");
		TSharedPtr<IAssetViewport> ActiveViewport = LevelEditorModule.GetFirstActiveViewport();
		if (!ActiveViewport.IsValid())
		{
			OutError = TEXT("No active level viewport — cannot start in-viewport PIE.");
			return false;
		}

		FRequestPlaySessionParams SessionParams;
		SessionParams.WorldType = EPlaySessionWorldType::PlayInEditor;
		SessionParams.DestinationSlateViewport = ActiveViewport;
		TGuardValue<bool> UnattendedScriptGuard(
			GIsRunningUnattendedScript,
			bSuppressModals ? true : GIsRunningUnattendedScript);
		GUnrealEd->RequestPlaySession(SessionParams);
		GUnrealEd->StartQueuedPlaySessionRequest();
		return true;
	}

	bool StopPieInternal()
	{
		if (!GEditor)
		{
			return false;
		}
		const bool bWasRunning = FindActivePieWorld() != nullptr;
		if (bWasRunning)
		{
			GEditor->RequestEndPlayMap();
		}
		return bWasRunning;
	}

	void RunScripts(const TSharedPtr<FJsonObject>& Params, UWorld* PieWorld)
	{
		const TArray<TSharedPtr<FJsonValue>>* ConsoleScript = nullptr;
		if (Params.IsValid() && PieWorld && Params->TryGetArrayField(TEXT("console_script"), ConsoleScript) && ConsoleScript)
		{
			APlayerController* PlayerController = PieWorld->GetFirstPlayerController();
			for (const TSharedPtr<FJsonValue>& Value : *ConsoleScript)
			{
				FString Command;
				if (Value.IsValid() && Value->TryGetString(Command) && !Command.IsEmpty())
				{
					if (PlayerController) { PlayerController->ConsoleCommand(Command, true); }
					else if (GEngine) { GEngine->Exec(PieWorld, *Command); }
				}
			}
		}

		FString PythonScript;
		if (Params.IsValid() && Params->TryGetStringField(TEXT("python_script"), PythonScript) && !PythonScript.IsEmpty())
		{
			if (IPythonScriptPlugin* Python = IPythonScriptPlugin::Get())
			{
				if (Python->IsPythonAvailable())
				{
					FPythonCommandEx Command;
					Command.Command = PythonScript;
					Command.ExecutionMode = EPythonCommandExecutionMode::ExecuteFile;
					Python->ExecPythonCommandEx(Command);
				}
			}
		}
	}

	FVector ParseVec3(const TArray<TSharedPtr<FJsonValue>>& Values, const FVector& Fallback)
	{
		FVector Result = Fallback;
		if (Values.IsValidIndex(0) && Values[0].IsValid()) { Result.X = Values[0]->AsNumber(); }
		if (Values.IsValidIndex(1) && Values[1].IsValid()) { Result.Y = Values[1]->AsNumber(); }
		if (Values.IsValidIndex(2) && Values[2].IsValid()) { Result.Z = Values[2]->AsNumber(); }
		return Result;
	}

	TArray<FAxonPieProvocation> ResolveProvocations(const TSharedPtr<FJsonObject>& Params)
	{
		TArray<FAxonPieProvocation> Result;
		const TArray<TSharedPtr<FJsonValue>>* Entries = nullptr;
		if (!Params.IsValid() || !Params->TryGetArrayField(TEXT("provocations"), Entries) || !Entries)
		{
			return Result;
		}

		for (const TSharedPtr<FJsonValue>& Entry : *Entries)
		{
			const TSharedPtr<FJsonObject>* Object = nullptr;
			if (!Entry.IsValid() || !Entry->TryGetObject(Object) || !Object || !Object->IsValid())
			{
				continue;
			}

			FAxonPieProvocation Provocation;
			(*Object)->TryGetNumberField(TEXT("time"), Provocation.AtSeconds);
			Provocation.AtSeconds = FMath::Max(0.0, Provocation.AtSeconds);
			(*Object)->TryGetStringField(TEXT("action"), Provocation.RawAction);

			const TSharedPtr<FJsonObject>* ActionParams = nullptr;
			const bool bHasParams = (*Object)->TryGetObjectField(TEXT("params"), ActionParams) &&
				ActionParams && ActionParams->IsValid();
			if (Provocation.RawAction.Equals(TEXT("set_control_rotation"), ESearchCase::IgnoreCase))
			{
				Provocation.Action = EAxonPieProvocationAction::SetControlRotation;
				if (bHasParams)
				{
					double Pitch = 0.0, Yaw = 0.0, Roll = 0.0;
					(*ActionParams)->TryGetNumberField(TEXT("pitch"), Pitch);
					(*ActionParams)->TryGetNumberField(TEXT("yaw"), Yaw);
					(*ActionParams)->TryGetNumberField(TEXT("roll"), Roll);
					Provocation.Rotation = FRotator(Pitch, Yaw, Roll);
				}
			}
			else if (Provocation.RawAction.Equals(TEXT("add_movement_input"), ESearchCase::IgnoreCase))
			{
				Provocation.Action = EAxonPieProvocationAction::AddMovementInput;
				if (bHasParams)
				{
					const TArray<TSharedPtr<FJsonValue>>* Direction = nullptr;
					if ((*ActionParams)->TryGetArrayField(TEXT("direction"), Direction) && Direction)
					{
						Provocation.Direction = ParseVec3(*Direction, FVector::ForwardVector);
					}
					(*ActionParams)->TryGetNumberField(TEXT("scale"), Provocation.Scale);
				}
			}
			else if (Provocation.RawAction.Equals(TEXT("jump"), ESearchCase::IgnoreCase))
			{
				Provocation.Action = EAxonPieProvocationAction::Jump;
			}
			else if (Provocation.RawAction.Equals(TEXT("console_command"), ESearchCase::IgnoreCase))
			{
				Provocation.Action = EAxonPieProvocationAction::ConsoleCommand;
				if (bHasParams)
				{
					(*ActionParams)->TryGetStringField(TEXT("command"), Provocation.Command);
				}
			}
			Result.Add(MoveTemp(Provocation));
		}
		return Result;
	}

	void ReadStringArrayField(const TSharedPtr<FJsonObject>& Params, const TCHAR* FieldName, TArray<FString>& OutValues)
	{
		const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
		if (!Params.IsValid() || !Params->TryGetArrayField(FieldName, Values) || !Values)
		{
			return;
		}
		for (const TSharedPtr<FJsonValue>& Value : *Values)
		{
			FString Text;
			if (Value.IsValid() && Value->TryGetString(Text) && !Text.IsEmpty())
			{
				OutValues.AddUnique(Text);
			}
		}
	}
}

void FAxonPieActions::RegisterActions(FAxonToolRegistry& Registry)
{
	const TSharedPtr<FJsonObject> TimeseriesSchema = FParamSchemaBuilder()
		.Optional(TEXT("actor"), TEXT("string"), TEXT("Exact editor label of the target actor."))
		.Optional(TEXT("pawn_class"), TEXT("string"), TEXT("Substring of the target actor class name."))
		.Optional(TEXT("object_name"), TEXT("string"), TEXT("Exact target actor object name. One actor selector is required."))
		.Optional(TEXT("component_name"), TEXT("string"), TEXT("Optional component on the resolved actor."))
		.Optional(TEXT("anim_instance"), TEXT("bool"), TEXT("Read the active skeletal-mesh anim instance."), TEXT("false"))
		.Required(TEXT("variables"), TEXT("array"), TEXT("Non-empty dotted variable paths to sample."))
		.OptionalAssetPath(TEXT("map"), TEXT("Level asset path to load before PIE."))
		.Optional(TEXT("duration_seconds"), TEXT("number"), TEXT("Session duration in seconds, clamped to 0-120."), TEXT("6"))
		.Optional(TEXT("sample_interval"), TEXT("number"), TEXT("Minimum seconds between samples."), TEXT("0"))
		.Optional(TEXT("max_samples"), TEXT("number"), TEXT("Maximum accumulated samples, clamped to 1-100000."), TEXT("2048"))
		.Optional(TEXT("provocations"), TEXT("array"), TEXT("Timed state changes: set_control_rotation, add_movement_input, jump, or console_command."))
		.Optional(TEXT("console_script"), TEXT("array"), TEXT("Console command strings run when PIE starts."))
		.Optional(TEXT("python_script"), TEXT("string"), TEXT("Python source run when PIE starts, if Python is available."))
		.Optional(TEXT("on_compile_errors"), TEXT("string"), TEXT("refuse (default) or suppress unresolved Blueprint compile-error prompts."), TEXT("refuse"))
		.Build();

	const FString Description = TEXT("Start an asynchronous PIE time-series sampling session. Returns immediately; poll editor.poll_pie_smoke for progress and samples, or stop it with editor.stop_pie_smoke. Samples dotted AxonStructField-compatible paths from a selected PIE actor, component, or AnimInstance and can fire timed provocations.");
	Registry.RegisterAction(TEXT("editor"), TEXT("sample_pie_timeseries"), Description,
		FAxonActionHandler::CreateStatic(&HandleSamplePieTimeseries), TimeseriesSchema);
	Registry.RegisterAction(TEXT("animation"), TEXT("sample_pie_timeseries"), Description,
		FAxonActionHandler::CreateStatic(&HandleSamplePieTimeseries), TimeseriesSchema);
	Registry.RegisterAction(TEXT("editor"), TEXT("poll_pie_smoke"),
		TEXT("Poll an Axon asynchronous PIE time-series session. Include the accumulated time series with include_samples=true, or it is included automatically after completion."),
		FAxonActionHandler::CreateStatic(&HandlePollPieSmoke),
		FParamSchemaBuilder()
			.Required(TEXT("session_id"), TEXT("string"), TEXT("Session id returned by sample_pie_timeseries."))
			.Optional(TEXT("include_samples"), TEXT("bool"), TEXT("Include full samples while the session is running."), TEXT("false"))
			.Build());
	Registry.RegisterAction(TEXT("editor"), TEXT("stop_pie_smoke"),
		TEXT("Stop one Axon PIE time-series session, or all running sessions when session_id is omitted."),
		FAxonActionHandler::CreateStatic(&HandleStopPieSmoke),
		FParamSchemaBuilder()
			.Optional(TEXT("session_id"), TEXT("string"), TEXT("Session id to stop; omit to stop all running sessions."))
			.Build());
	Registry.RegisterAction(TEXT("editor"), TEXT("start_pie"),
		TEXT("Start an in-viewport Play-In-Editor session."),
		FAxonActionHandler::CreateStatic(&HandleStartPie), MakeShared<FJsonObject>());
	Registry.RegisterAction(TEXT("editor"), TEXT("stop_pie"),
		TEXT("Stop the active Play-In-Editor session."),
		FAxonActionHandler::CreateStatic(&HandleStopPie), MakeShared<FJsonObject>());
}

void FAxonPieActions::UnregisterActions(FAxonToolRegistry& Registry)
{
	Registry.UnregisterAction(TEXT("editor"), TEXT("sample_pie_timeseries"));
	Registry.UnregisterAction(TEXT("animation"), TEXT("sample_pie_timeseries"));
	Registry.UnregisterAction(TEXT("editor"), TEXT("poll_pie_smoke"));
	Registry.UnregisterAction(TEXT("editor"), TEXT("stop_pie_smoke"));
	Registry.UnregisterAction(TEXT("editor"), TEXT("start_pie"));
	Registry.UnregisterAction(TEXT("editor"), TEXT("stop_pie"));
}

FAxonActionResult FAxonPieActions::HandleSamplePieTimeseries(const TSharedPtr<FJsonObject>& Params)
{
	if (!GEditor || !GUnrealEd)
	{
		return FAxonActionResult::Error(TEXT("sample_pie_timeseries requires editor context (GEditor/GUnrealEd)."));
	}

	FString ActorLabel, ObjectName, ClassName;
	Params->TryGetStringField(TEXT("actor"), ActorLabel);
	Params->TryGetStringField(TEXT("object_name"), ObjectName);
	Params->TryGetStringField(TEXT("pawn_class"), ClassName);
	if (ActorLabel.IsEmpty() && ObjectName.IsEmpty() && ClassName.IsEmpty())
	{
		return FAxonActionResult::Error(TEXT("sample_pie_timeseries requires one of: actor, pawn_class, object_name."));
	}

	TArray<FString> Variables;
	ReadStringArrayField(Params, TEXT("variables"), Variables);
	if (Variables.IsEmpty())
	{
		return FAxonActionResult::Error(TEXT("sample_pie_timeseries requires a non-empty variables array."));
	}

	double Duration = 6.0;
	Params->TryGetNumberField(TEXT("duration_seconds"), Duration);
	Duration = FMath::Clamp(Duration, 0.0, 120.0);
	double SampleInterval = 0.0;
	Params->TryGetNumberField(TEXT("sample_interval"), SampleInterval);
	SampleInterval = FMath::Max(0.0, SampleInterval);
	double MaxSamplesValue = 2048.0;
	Params->TryGetNumberField(TEXT("max_samples"), MaxSamplesValue);
	const int32 MaxSamples = FMath::Clamp(static_cast<int32>(MaxSamplesValue), 1, 100000);

	if (FindActivePieWorld())
	{
		return FAxonActionResult::Error(TEXT("A PIE session is already running — stop it before sample_pie_timeseries."));
	}

	FString LoadError;
	if (!LoadMapIfRequested(Params, LoadError))
	{
		return FAxonActionResult::Error(LoadError);
	}

	FString CompileMode = TEXT("refuse");
	Params->TryGetStringField(TEXT("on_compile_errors"), CompileMode);
	const bool bSuppressModals = CompileMode.Equals(TEXT("suppress"), ESearchCase::IgnoreCase);
	TArray<FErroredBlueprintEntry> ErroredBlueprints;
	ScanErroredBlueprints(ErroredBlueprints);
	if (!bSuppressModals && !ErroredBlueprints.IsEmpty())
	{
		TSharedPtr<FJsonObject> ErrorData = MakeShared<FJsonObject>();
		ErrorData->SetNumberField(TEXT("errored_blueprint_count"), ErroredBlueprints.Num());
		ErrorData->SetArrayField(TEXT("errored_blueprints"), ErroredBlueprintsToJson(ErroredBlueprints));
		return FAxonActionResult::Error(FString::Printf(
			TEXT("sample_pie_timeseries refused: %d Blueprint(s) have unresolved compile errors. Fix them or pass on_compile_errors=\"suppress\"."),
			ErroredBlueprints.Num())).WithErrorData(ErrorData);
	}

	FString StartError;
	if (!StartPieInternal(StartError, bSuppressModals))
	{
		return FAxonActionResult::Error(FString::Printf(TEXT("Failed to start PIE: %s"), *StartError));
	}

	UWorld* PieWorld = FindActivePieWorld();
	if (PieWorld)
	{
		RunScripts(Params, PieWorld);
	}

	FAxonPieSession Session;
	Session.StartTimeSeconds = FPlatformTime::Seconds();
	Session.DurationSeconds = Duration;
	Session.SampleInterval = SampleInterval;
	Session.MaxSamples = MaxSamples;
	Session.MapName = PieWorld ? PieWorld->GetMapName() : TEXT("<current>");
	Session.TargetActorLabel = ActorLabel;
	Session.TargetObjectName = ObjectName;
	Session.TargetClassName = ClassName;
	Session.VariablePaths = MoveTemp(Variables);
	Params->TryGetStringField(TEXT("component_name"), Session.TargetComponentName);
	Params->TryGetBoolField(TEXT("anim_instance"), Session.bTargetAnimInstance);
	Session.Provocations = ResolveProvocations(Params);

	const FString SessionId = FAxonPieSessionManager::Get().CreateSession(MoveTemp(Session));
	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetStringField(TEXT("session_id"), SessionId);
	Result->SetStringField(TEXT("status"), TEXT("running"));
	Result->SetBoolField(TEXT("started"), true);
	Result->SetNumberField(TEXT("duration"), Duration);
	Result->SetStringField(TEXT("note"), TEXT("Poll with editor.poll_pie_smoke; stop with editor.stop_pie_smoke."));
	return FAxonActionResult::Success(Result);
}

FAxonActionResult FAxonPieActions::HandlePollPieSmoke(const TSharedPtr<FJsonObject>& Params)
{
	FString SessionId;
	if (!Params->TryGetStringField(TEXT("session_id"), SessionId) || SessionId.IsEmpty())
	{
		return FAxonActionResult::Error(TEXT("poll_pie_smoke requires a session_id."));
	}
	FAxonPieSessionManager& Manager = FAxonPieSessionManager::Get();
	FAxonPieSession* Session = Manager.Find(SessionId);
	if (!Session)
	{
		return FAxonActionResult::Error(FString::Printf(TEXT("Unknown Axon PIE session '%s'."), *SessionId));
	}
	bool bIncludeSamples = false;
	Params->TryGetBoolField(TEXT("include_samples"), bIncludeSamples);
	return FAxonActionResult::Success(Manager.BuildReport(
		*Session, bIncludeSamples || Session->Status != EAxonPieSessionStatus::Running));
}

FAxonActionResult FAxonPieActions::HandleStopPieSmoke(const TSharedPtr<FJsonObject>& Params)
{
	FString SessionId;
	Params->TryGetStringField(TEXT("session_id"), SessionId);
	FAxonPieSessionManager& Manager = FAxonPieSessionManager::Get();
	const int32 Stopped = Manager.Stop(SessionId);
	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetNumberField(TEXT("stopped"), Stopped);
	if (!SessionId.IsEmpty())
	{
		if (FAxonPieSession* Session = Manager.Find(SessionId))
		{
			Result->SetObjectField(TEXT("report"), Manager.BuildReport(*Session, true));
		}
		else
		{
			Result->SetStringField(TEXT("warning"), FString::Printf(TEXT("Unknown session '%s' — nothing to stop."), *SessionId));
		}
	}
	return FAxonActionResult::Success(Result);
}

FAxonActionResult FAxonPieActions::HandleStartPie(const TSharedPtr<FJsonObject>&)
{
	if (FindActivePieWorld())
	{
		TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
		Result->SetBoolField(TEXT("started"), false);
		Result->SetStringField(TEXT("reason"), TEXT("PIE already running"));
		return FAxonActionResult::Success(Result);
	}
	FString Error;
	if (!StartPieInternal(Error))
	{
		return FAxonActionResult::Error(Error);
	}
	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("started"), true);
	Result->SetStringField(TEXT("mode"), TEXT("in_viewport"));
	return FAxonActionResult::Success(Result);
}

FAxonActionResult FAxonPieActions::HandleStopPie(const TSharedPtr<FJsonObject>&)
{
	if (!GEditor)
	{
		return FAxonActionResult::Error(TEXT("GEditor not available"));
	}
	TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
	Result->SetBoolField(TEXT("stopped"), StopPieInternal());
	return FAxonActionResult::Success(Result);
}
