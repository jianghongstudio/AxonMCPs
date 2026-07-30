#include "AxonRewindDebuggerActions.h"
#include "AxonRewindDebuggerUtils.h"
#include "AxonParamSchema.h"
#include "AxonJsonUtils.h"
#include "IRewindDebugger.h"
#include "Engine/Engine.h"
#include "RewindDebuggerTrack.h"
#include "RewindDebuggerRuntime/RewindDebuggerRuntime.h"
#include "IAnimationProvider.h"
#include "IGameplayProvider.h"
#include "TraceServices/Model/AnalysisSession.h"
#include "TraceServices/Model/Frames.h"

#if WITH_CAMERA_BLUEPRINT
#include "Trace/ICameraTraceProvider.h"
#endif

using namespace AxonRewindDebugger;

namespace
{
	static void Register(FAxonToolRegistry& R, const TCHAR* Name, const TCHAR* Desc,
		FAxonActionResult(*Handler)(const TSharedPtr<FJsonObject>&), TSharedPtr<FJsonObject> Schema)
	{
		R.RegisterAction(TEXT("rewind_debugger"), Name, Desc, FAxonActionHandler::CreateStatic(Handler), Schema);
	}

	static TSharedPtr<FJsonObject> ObjectIdSchema(bool bRequired = true)
	{
		FParamSchemaBuilder B;
		if (bRequired)
		{
			B.Required(TEXT("object_id"), TEXT("string"), TEXT("Trace object id (uint64 as string)"));
		}
		else
		{
			B.Optional(TEXT("object_id"), TEXT("string"), TEXT("Trace object id (uint64 as string)"));
		}
		B.Optional(TEXT("time"), TEXT("number"), TEXT("Optional recording scrub time (seconds); moves scrub when sampling"));
		B.Optional(TEXT("frame_window"), TEXT("number"), TEXT("Optional seconds around scrub for event window (default: game frame)"));
		B.Optional(TEXT("limit"), TEXT("number"), TEXT("Max events to return"), LexToString(DefaultLimit));
		return B.Build();
	}

	static TSharedPtr<FJsonObject> SessionMeta(const FSampleContext& Ctx)
	{
		TSharedPtr<FJsonObject> J = MakeShared<FJsonObject>();
		J->SetNumberField(TEXT("scrub_time"), Ctx.ScrubTime);
		J->SetNumberField(TEXT("trace_time"), Ctx.TraceTime);
		J->SetNumberField(TEXT("start_time"), Ctx.StartTime);
		J->SetNumberField(TEXT("end_time"), Ctx.EndTime);
		J->SetBoolField(TEXT("moved_scrub"), Ctx.bMovedScrub);
		return J;
	}

	static FAxonActionResult NeedAnim(const FSampleContext& Ctx)
	{
		if (!Ctx.AnimProvider)
		{
			return FAxonActionResult::Error(TEXT("AnimationProvider not present in analysis session"));
		}
		return FAxonActionResult::Success(MakeShared<FJsonObject>());
	}
}

void FAxonRewindDebuggerActions::RegisterActions(FAxonToolRegistry& R)
{
	Register(R, TEXT("get_session_info"),
		TEXT("Rewind Debugger session: recording state, scrub/trace time, duration, root object"),
		&HandleGetSessionInfo, FParamSchemaBuilder().Build());

	Register(R, TEXT("list_debugged_objects"),
		TEXT("List objects currently attached to Rewind Debugger tracks (flat tree with depth)"),
		&HandleListDebuggedObjects,
		FParamSchemaBuilder().Optional(TEXT("limit"), TEXT("number"), TEXT("Max objects"), TEXT("200")).Build());

	Register(R, TEXT("get_selected_track"),
		TEXT("Get the currently selected Rewind Debugger track (name, display name, object id)"),
		&HandleGetSelectedTrack, FParamSchemaBuilder().Build());

	Register(R, TEXT("set_scrub_time"),
		TEXT("Document scrub write gap: public API cannot move UI scrub; pass time= to sample_* instead"),
		&HandleSetScrubTime,
		FParamSchemaBuilder().Required(TEXT("time"), TEXT("number"), TEXT("Recording scrub time in seconds")).Build());

	Register(R, TEXT("start_recording"),
		TEXT("Start Rewind Debugger recording (requires PIE simulating)"),
		&HandleStartRecording, FParamSchemaBuilder().Build());

	Register(R, TEXT("stop_recording"),
		TEXT("Stop Rewind Debugger recording via FRewindDebugger / Runtime"),
		&HandleStopRecording, FParamSchemaBuilder().Build());

	Register(R, TEXT("list_anim_instances"),
		TEXT("List AnimInstance object ids that have AnimGraph timelines in the session"),
		&HandleListAnimInstances,
		FParamSchemaBuilder()
			.Optional(TEXT("limit"), TEXT("number"), TEXT("Max instances"), LexToString(DefaultLimit))
			.Build());

	Register(R, TEXT("sample_anim_nodes"),
		TEXT("Sample AnimNode events for an AnimInstance at scrub/frame"),
		&HandleSampleAnimNodes, ObjectIdSchema());

	Register(R, TEXT("sample_anim_node_values"),
		TEXT("Sample AnimNode debug values for an AnimInstance at scrub/frame"),
		&HandleSampleAnimNodeValues, ObjectIdSchema());

	Register(R, TEXT("sample_state_machines"),
		TEXT("Sample state machine messages for an AnimInstance at scrub/frame"),
		&HandleSampleStateMachines, ObjectIdSchema());

	Register(R, TEXT("sample_montages"),
		TEXT("Sample montage timeline for an AnimInstance at scrub/frame"),
		&HandleSampleMontages, ObjectIdSchema());

	Register(R, TEXT("sample_sequence_players"),
		TEXT("Sample AnimSequencePlayer messages for an AnimInstance at scrub/frame"),
		&HandleSampleSequencePlayers, ObjectIdSchema());

	Register(R, TEXT("sample_skeletal_pose"),
		TEXT("Sample skeletal mesh pose SUMMARY (bone count + root). Opt-in include_full_pose with hard bone limit"),
		&HandleSampleSkeletalPose,
		FParamSchemaBuilder()
			.Required(TEXT("object_id"), TEXT("string"), TEXT("SkeletalMeshComponent object id"))
			.Optional(TEXT("time"), TEXT("number"), TEXT("Optional recording scrub time (seconds)"))
			.Optional(TEXT("frame_window"), TEXT("number"), TEXT("Optional seconds around scrub"))
			.Optional(TEXT("include_full_pose"), TEXT("boolean"), TEXT("Include bone transforms (capped)"), TEXT("false"))
			.Optional(TEXT("limit"), TEXT("number"), TEXT("Max bones when include_full_pose"), LexToString(MaxPoseBones))
			.Build());

	Register(R, TEXT("sample_camera_graph_result"),
		TEXT("Sample CameraBP CameraGraphResult at scrub/frame (requires CameraBlueprint)"),
		&HandleSampleCameraGraphResult, ObjectIdSchema());

	Register(R, TEXT("sample_camera_watches"),
		TEXT("Sample CameraBP CameraWatch events at scrub/frame (requires CameraBlueprint)"),
		&HandleSampleCameraWatches, ObjectIdSchema());

	Register(R, TEXT("resolve_object"),
		TEXT("Resolve a trace object_id to class/name/outer via IGameplayProvider"),
		&HandleResolveObject,
		FParamSchemaBuilder().Required(TEXT("object_id"), TEXT("string"), TEXT("Trace object id")).Build());
}

FAxonActionResult FAxonRewindDebuggerActions::HandleGetSessionInfo(const TSharedPtr<FJsonObject>&)
{
	IRewindDebugger* Debugger = IRewindDebugger::Instance();
	if (!Debugger)
	{
		return FAxonActionResult::Error(TEXT("Rewind Debugger is not available"));
	}

	TSharedPtr<FJsonObject> J = MakeShared<FJsonObject>();
	J->SetBoolField(TEXT("has_instance"), true);
	J->SetBoolField(TEXT("is_recording"), Debugger->IsRecording());
	J->SetBoolField(TEXT("is_pie_simulating"), Debugger->IsPIESimulating());
	J->SetBoolField(TEXT("is_trace_file_loaded"), Debugger->IsTraceFileLoaded());
	J->SetBoolField(TEXT("can_start_recording"), Debugger->CanStartRecording());
	J->SetNumberField(TEXT("scrub_time"), Debugger->GetScrubTime());
	J->SetNumberField(TEXT("trace_time"), Debugger->CurrentTraceTime());
	J->SetNumberField(TEXT("recording_duration"), Debugger->GetRecordingDuration());
	J->SetStringField(TEXT("root_object_id"), LexToString(Debugger->GetRootObjectId()));

	const TRange<double>& View = Debugger->GetCurrentViewRange();
	J->SetNumberField(TEXT("view_range_min"), View.GetLowerBoundValue());
	J->SetNumberField(TEXT("view_range_max"), View.GetUpperBoundValue());

	const TRange<double>& TraceRange = Debugger->GetCurrentTraceRange();
	J->SetNumberField(TEXT("trace_range_min"), TraceRange.GetLowerBoundValue());
	J->SetNumberField(TEXT("trace_range_max"), TraceRange.GetUpperBoundValue());

	J->SetBoolField(TEXT("has_analysis_session"), Debugger->GetAnalysisSession() != nullptr);

	if (UWorld* World = Debugger->GetWorldToVisualize())
	{
		J->SetStringField(TEXT("world"), World->GetPathName());
	}

	if (TSharedPtr<FDebugObjectInfo> Selected = Debugger->GetSelectedObject())
	{
		J->SetStringField(TEXT("selected_object_id"), LexToString(Selected->GetUObjectId()));
		J->SetStringField(TEXT("selected_object_name"), Selected->ObjectName);
	}

	return FAxonActionResult::Success(J);
}

FAxonActionResult FAxonRewindDebuggerActions::HandleListDebuggedObjects(const TSharedPtr<FJsonObject>& Params)
{
	IRewindDebugger* Debugger = IRewindDebugger::Instance();
	if (!Debugger)
	{
		return FAxonActionResult::Error(TEXT("Rewind Debugger is not available"));
	}

	int32 Remaining = ParseLimit(Params, 200);
	TArray<TSharedPtr<FJsonValue>> Objects;
	for (const TSharedPtr<FDebugObjectInfo>& Root : Debugger->GetDebuggedObjects())
	{
		AppendDebugObjectTree(Root, Objects, 0, Remaining);
	}

	TSharedPtr<FJsonObject> J = MakeShared<FJsonObject>();
	J->SetArrayField(TEXT("objects"), Objects);
	J->SetNumberField(TEXT("count"), Objects.Num());
	return FAxonActionResult::Success(J);
}

FAxonActionResult FAxonRewindDebuggerActions::HandleGetSelectedTrack(const TSharedPtr<FJsonObject>&)
{
	IRewindDebugger* Debugger = IRewindDebugger::Instance();
	if (!Debugger)
	{
		return FAxonActionResult::Error(TEXT("Rewind Debugger is not available"));
	}

	TSharedPtr<RewindDebugger::FRewindDebuggerTrack> Track = Debugger->GetSelectedTrack();
	TSharedPtr<FJsonObject> J = MakeShared<FJsonObject>();
	J->SetBoolField(TEXT("has_selection"), Track.IsValid());
	if (Track.IsValid())
	{
		J->SetStringField(TEXT("name"), Track->GetName().ToString());
		J->SetStringField(TEXT("display_name"), Track->GetDisplayName().ToString());
		J->SetStringField(TEXT("object_id"), LexToString(Track->GetUObjectId()));
	}
	return FAxonActionResult::Success(J);
}

FAxonActionResult FAxonRewindDebuggerActions::HandleSetScrubTime(const TSharedPtr<FJsonObject>& Params)
{
	double Time = 0.0;
	if (!Params.IsValid() || !Params->TryGetNumberField(TEXT("time"), Time))
	{
		return FAxonActionResult::Error(TEXT("time (number, recording seconds) is required"), FAxonJsonUtils::ErrInvalidParams);
	}

	IRewindDebugger* Debugger = IRewindDebugger::Instance();
	if (!Debugger)
	{
		return FAxonActionResult::Error(TEXT("Rewind Debugger is not available"));
	}

	// FRewindDebugger::ScrubToTime is not DLL-exported. Sample_* accept optional time= instead.
	TSharedPtr<FJsonObject> J = MakeShared<FJsonObject>();
	J->SetBoolField(TEXT("scrubbed"), false);
	J->SetBoolField(TEXT("ui_scrub_supported"), false);
	J->SetNumberField(TEXT("requested_time"), Time);
	J->SetNumberField(TEXT("current_scrub_time"), Debugger->GetScrubTime());
	J->SetNumberField(TEXT("current_trace_time"), Debugger->CurrentTraceTime());
	J->SetStringField(TEXT("note"), TEXT("IRewindDebugger has no public scrub write API. Pass time= to sample_* to resolve RecordingInfo to profile time without moving the UI scrubber."));
	return FAxonActionResult::Success(J);
}

FAxonActionResult FAxonRewindDebuggerActions::HandleStartRecording(const TSharedPtr<FJsonObject>&)
{
	IRewindDebugger* Debugger = IRewindDebugger::Instance();
	if (!Debugger)
	{
		return FAxonActionResult::Error(TEXT("Rewind Debugger is not available"));
	}

	TSharedPtr<FJsonObject> J = MakeShared<FJsonObject>();
	if (Debugger->IsRecording())
	{
		J->SetBoolField(TEXT("started"), false);
		J->SetBoolField(TEXT("already_recording"), true);
		return FAxonActionResult::Success(J);
	}

	if (!Debugger->CanStartRecording())
	{
		return FAxonActionResult::Error(TEXT("Cannot start recording (PIE must be simulating)"));
	}

	Debugger->StartRecording();
	J->SetBoolField(TEXT("started"), Debugger->IsRecording());
	J->SetBoolField(TEXT("is_recording"), Debugger->IsRecording());
	return FAxonActionResult::Success(J);
}

FAxonActionResult FAxonRewindDebuggerActions::HandleStopRecording(const TSharedPtr<FJsonObject>&)
{
	TSharedPtr<FJsonObject> J = MakeShared<FJsonObject>();

	if (RewindDebugger::FRewindDebuggerRuntime* Runtime = RewindDebugger::FRewindDebuggerRuntime::Instance())
	{
		const bool bWas = Runtime->IsRecording();
		if (bWas)
		{
			Runtime->StopRecording();
		}
		J->SetBoolField(TEXT("stopped"), bWas && !Runtime->IsRecording());
		J->SetBoolField(TEXT("was_recording"), bWas);
		J->SetBoolField(TEXT("is_recording"), Runtime->IsRecording());
		J->SetStringField(TEXT("via"), TEXT("FRewindDebuggerRuntime::StopRecording"));
		return FAxonActionResult::Success(J);
	}

	if (GEngine)
	{
		GEngine->Exec(nullptr, TEXT("RewindDebugger.StopRecording"));
		J->SetBoolField(TEXT("stopped"), true);
		J->SetStringField(TEXT("via"), TEXT("console RewindDebugger.StopRecording"));
		if (IRewindDebugger* Debugger = IRewindDebugger::Instance())
		{
			J->SetBoolField(TEXT("is_recording"), Debugger->IsRecording());
		}
		return FAxonActionResult::Success(J);
	}

	return FAxonActionResult::Error(TEXT("FRewindDebuggerRuntime is not available"));
}

FAxonActionResult FAxonRewindDebuggerActions::HandleListAnimInstances(const TSharedPtr<FJsonObject>& Params)
{
	FSampleContext Ctx;
	if (FString Err = ResolveSampleContext(Params, Ctx, false); !Err.IsEmpty())
	{
		return FAxonActionResult::Error(Err);
	}
	if (FAxonActionResult E = NeedAnim(Ctx); !E.bSuccess) return E;

	TraceServices::FAnalysisSessionReadScope SessionReadScope(*Ctx.Session);
	TArray<TSharedPtr<FJsonValue>> Instances;
	int32 Remaining = Ctx.Limit;
	Ctx.AnimProvider->EnumerateAnimGraphTimelines([&](uint64 ObjectId, const IAnimationProvider::AnimGraphTimeline&)
	{
		if (Remaining <= 0) return;
		TSharedPtr<FJsonObject> Row = MakeObjectInfoJson(Ctx.GameplayProvider, ObjectId);
		Instances.Add(MakeShared<FJsonValueObject>(Row));
		--Remaining;
	});

	TSharedPtr<FJsonObject> J = MakeShared<FJsonObject>();
	J->SetArrayField(TEXT("anim_instances"), Instances);
	J->SetNumberField(TEXT("count"), Instances.Num());
	J->SetObjectField(TEXT("sample"), SessionMeta(Ctx));
	return FAxonActionResult::Success(J);
}

FAxonActionResult FAxonRewindDebuggerActions::HandleSampleAnimNodes(const TSharedPtr<FJsonObject>& Params)
{
	uint64 ObjectId = 0; FString IdErr;
	if (!ParseObjectId(Params, ObjectId, IdErr)) return FAxonActionResult::Error(IdErr, FAxonJsonUtils::ErrInvalidParams);

	FSampleContext Ctx;
	if (FString Err = ResolveSampleContext(Params, Ctx); !Err.IsEmpty()) return FAxonActionResult::Error(Err);
	if (FAxonActionResult E = NeedAnim(Ctx); !E.bSuccess) return E;

	TraceServices::FAnalysisSessionReadScope SessionReadScope(*Ctx.Session);
	TArray<TSharedPtr<FJsonValue>> Events;
	int32 Remaining = Ctx.Limit;
	const bool bOk = Ctx.AnimProvider->ReadAnimNodesTimeline(ObjectId, [&](const IAnimationProvider::AnimNodesTimeline& Timeline)
	{
		Timeline.EnumerateEvents(Ctx.StartTime, Ctx.EndTime,
			[&](double StartTime, double EndTime, uint32 /*Depth*/, const FAnimNodeMessage& Message)
			{
				if (Remaining <= 0) return TraceServices::EEventEnumerate::Stop;
				TSharedPtr<FJsonObject> Row = MakeShared<FJsonObject>();
				Row->SetNumberField(TEXT("node_id"), Message.NodeId);
				Row->SetNumberField(TEXT("previous_node_id"), Message.PreviousNodeId);
				Row->SetStringField(TEXT("node_name"), Message.NodeName ? Message.NodeName : TEXT(""));
				Row->SetStringField(TEXT("node_type"), Message.NodeTypeName ? Message.NodeTypeName : TEXT(""));
				Row->SetNumberField(TEXT("weight"), Message.Weight);
				Row->SetNumberField(TEXT("root_motion_weight"), Message.RootMotionWeight);
				Row->SetNumberField(TEXT("frame_counter"), Message.FrameCounter);
				Row->SetNumberField(TEXT("phase"), static_cast<int32>(Message.Phase));
				Row->SetNumberField(TEXT("start_time"), StartTime);
				Row->SetNumberField(TEXT("end_time"), EndTime);
				if (const FAnimNodeInfo* Info = Ctx.AnimProvider->FindAnimNodeInfo(Message.NodeId, ObjectId))
				{
					Row->SetStringField(TEXT("info_name"), Info->Name ? Info->Name : TEXT(""));
					Row->SetStringField(TEXT("info_type"), Info->TypeName ? Info->TypeName : TEXT(""));
				}
				Events.Add(MakeShared<FJsonValueObject>(Row));
				--Remaining;
				return TraceServices::EEventEnumerate::Continue;
			});
	});

	TSharedPtr<FJsonObject> J = MakeShared<FJsonObject>();
	J->SetStringField(TEXT("object_id"), LexToString(ObjectId));
	J->SetBoolField(TEXT("has_timeline"), bOk);
	J->SetArrayField(TEXT("nodes"), Events);
	J->SetNumberField(TEXT("count"), Events.Num());
	J->SetObjectField(TEXT("sample"), SessionMeta(Ctx));
	return FAxonActionResult::Success(J);
}

FAxonActionResult FAxonRewindDebuggerActions::HandleSampleAnimNodeValues(const TSharedPtr<FJsonObject>& Params)
{
	uint64 ObjectId = 0; FString IdErr;
	if (!ParseObjectId(Params, ObjectId, IdErr)) return FAxonActionResult::Error(IdErr, FAxonJsonUtils::ErrInvalidParams);

	FSampleContext Ctx;
	if (FString Err = ResolveSampleContext(Params, Ctx); !Err.IsEmpty()) return FAxonActionResult::Error(Err);
	if (FAxonActionResult E = NeedAnim(Ctx); !E.bSuccess) return E;

	TraceServices::FAnalysisSessionReadScope SessionReadScope(*Ctx.Session);
	TArray<TSharedPtr<FJsonValue>> Events;
	int32 Remaining = Ctx.Limit;
	const bool bOk = Ctx.AnimProvider->ReadAnimNodeValuesTimeline(ObjectId, [&](const IAnimationProvider::AnimNodeValuesTimeline& Timeline)
	{
		Timeline.EnumerateEvents(Ctx.StartTime, Ctx.EndTime,
			[&](double StartTime, double /*EndTime*/, uint32 /*Depth*/, const FAnimNodeValueMessage& Message)
			{
				if (Remaining <= 0) return TraceServices::EEventEnumerate::Stop;
				TSharedPtr<FJsonObject> Row = MakeShared<FJsonObject>();
				Row->SetNumberField(TEXT("node_id"), Message.NodeId);
				Row->SetStringField(TEXT("key"), Message.Key ? Message.Key : TEXT(""));
				Row->SetNumberField(TEXT("frame_counter"), Message.FrameCounter);
				Row->SetNumberField(TEXT("recording_time"), Message.RecordingTime);
				Row->SetNumberField(TEXT("time"), StartTime);
				Row->SetObjectField(TEXT("value"), MakeVariantValueJson(Message.Value, Ctx.AnimProvider));
				if (Ctx.AnimProvider)
				{
					Row->SetStringField(TEXT("formatted"), Ctx.AnimProvider->FormatNodeKeyValue(Message).ToString());
				}
				Events.Add(MakeShared<FJsonValueObject>(Row));
				--Remaining;
				return TraceServices::EEventEnumerate::Continue;
			});
	});

	TSharedPtr<FJsonObject> J = MakeShared<FJsonObject>();
	J->SetStringField(TEXT("object_id"), LexToString(ObjectId));
	J->SetBoolField(TEXT("has_timeline"), bOk);
	J->SetArrayField(TEXT("values"), Events);
	J->SetNumberField(TEXT("count"), Events.Num());
	J->SetObjectField(TEXT("sample"), SessionMeta(Ctx));
	return FAxonActionResult::Success(J);
}

FAxonActionResult FAxonRewindDebuggerActions::HandleSampleStateMachines(const TSharedPtr<FJsonObject>& Params)
{
	uint64 ObjectId = 0; FString IdErr;
	if (!ParseObjectId(Params, ObjectId, IdErr)) return FAxonActionResult::Error(IdErr, FAxonJsonUtils::ErrInvalidParams);

	FSampleContext Ctx;
	if (FString Err = ResolveSampleContext(Params, Ctx); !Err.IsEmpty()) return FAxonActionResult::Error(Err);
	if (FAxonActionResult E = NeedAnim(Ctx); !E.bSuccess) return E;

	TraceServices::FAnalysisSessionReadScope SessionReadScope(*Ctx.Session);
	TArray<TSharedPtr<FJsonValue>> Events;
	int32 Remaining = Ctx.Limit;
	const bool bOk = Ctx.AnimProvider->ReadStateMachinesTimeline(ObjectId, [&](const IAnimationProvider::StateMachinesTimeline& Timeline)
	{
		Timeline.EnumerateEvents(Ctx.StartTime, Ctx.EndTime,
			[&](double StartTime, double /*EndTime*/, uint32 /*Depth*/, const FAnimStateMachineMessage& Message)
			{
				if (Remaining <= 0) return TraceServices::EEventEnumerate::Stop;
				TSharedPtr<FJsonObject> Row = MakeShared<FJsonObject>();
				Row->SetNumberField(TEXT("node_id"), Message.NodeId);
				Row->SetNumberField(TEXT("state_machine_index"), Message.StateMachineIndex);
				Row->SetNumberField(TEXT("state_index"), Message.StateIndex);
				Row->SetNumberField(TEXT("state_weight"), Message.StateWeight);
				Row->SetNumberField(TEXT("elapsed_time"), Message.ElapsedTime);
				Row->SetNumberField(TEXT("time"), StartTime);
				Events.Add(MakeShared<FJsonValueObject>(Row));
				--Remaining;
				return TraceServices::EEventEnumerate::Continue;
			});
	});

	TSharedPtr<FJsonObject> J = MakeShared<FJsonObject>();
	J->SetStringField(TEXT("object_id"), LexToString(ObjectId));
	J->SetBoolField(TEXT("has_timeline"), bOk);
	J->SetArrayField(TEXT("state_machines"), Events);
	J->SetNumberField(TEXT("count"), Events.Num());
	J->SetObjectField(TEXT("sample"), SessionMeta(Ctx));
	return FAxonActionResult::Success(J);
}

FAxonActionResult FAxonRewindDebuggerActions::HandleSampleMontages(const TSharedPtr<FJsonObject>& Params)
{
	uint64 ObjectId = 0; FString IdErr;
	if (!ParseObjectId(Params, ObjectId, IdErr)) return FAxonActionResult::Error(IdErr, FAxonJsonUtils::ErrInvalidParams);

	FSampleContext Ctx;
	if (FString Err = ResolveSampleContext(Params, Ctx); !Err.IsEmpty()) return FAxonActionResult::Error(Err);
	if (FAxonActionResult E = NeedAnim(Ctx); !E.bSuccess) return E;

	TraceServices::FAnalysisSessionReadScope SessionReadScope(*Ctx.Session);
	TArray<TSharedPtr<FJsonValue>> Events;
	int32 Remaining = Ctx.Limit;
	const bool bOk = Ctx.AnimProvider->ReadMontageTimeline(ObjectId, [&](const IAnimationProvider::AnimMontageTimeline& Timeline)
	{
		Timeline.EnumerateEvents(Ctx.StartTime, Ctx.EndTime,
			[&](double StartTime, double /*EndTime*/, uint32 /*Depth*/, const FAnimMontageMessage& Message)
			{
				if (Remaining <= 0) return TraceServices::EEventEnumerate::Stop;
				TSharedPtr<FJsonObject> Row = MakeShared<FJsonObject>();
				Row->SetStringField(TEXT("montage_id"), LexToString(Message.MontageId));
				Row->SetNumberField(TEXT("weight"), Message.Weight);
				Row->SetNumberField(TEXT("desired_weight"), Message.DesiredWeight);
				Row->SetNumberField(TEXT("position"), Message.Position);
				Row->SetNumberField(TEXT("frame_counter"), Message.FrameCounter);
				Row->SetNumberField(TEXT("recording_time"), Message.RecordingTime);
				Row->SetNumberField(TEXT("time"), StartTime);
				if (Ctx.AnimProvider)
				{
					Row->SetStringField(TEXT("current_section"), Ctx.AnimProvider->GetName(Message.CurrentSectionNameId) ? Ctx.AnimProvider->GetName(Message.CurrentSectionNameId) : TEXT(""));
					Row->SetStringField(TEXT("next_section"), Ctx.AnimProvider->GetName(Message.NextSectionNameId) ? Ctx.AnimProvider->GetName(Message.NextSectionNameId) : TEXT(""));
				}
				Events.Add(MakeShared<FJsonValueObject>(Row));
				--Remaining;
				return TraceServices::EEventEnumerate::Continue;
			});
	});

	TSharedPtr<FJsonObject> J = MakeShared<FJsonObject>();
	J->SetStringField(TEXT("object_id"), LexToString(ObjectId));
	J->SetBoolField(TEXT("has_timeline"), bOk);
	J->SetArrayField(TEXT("montages"), Events);
	J->SetNumberField(TEXT("count"), Events.Num());
	J->SetObjectField(TEXT("sample"), SessionMeta(Ctx));
	return FAxonActionResult::Success(J);
}

FAxonActionResult FAxonRewindDebuggerActions::HandleSampleSequencePlayers(const TSharedPtr<FJsonObject>& Params)
{
	uint64 ObjectId = 0; FString IdErr;
	if (!ParseObjectId(Params, ObjectId, IdErr)) return FAxonActionResult::Error(IdErr, FAxonJsonUtils::ErrInvalidParams);

	FSampleContext Ctx;
	if (FString Err = ResolveSampleContext(Params, Ctx); !Err.IsEmpty()) return FAxonActionResult::Error(Err);
	if (FAxonActionResult E = NeedAnim(Ctx); !E.bSuccess) return E;

	TraceServices::FAnalysisSessionReadScope SessionReadScope(*Ctx.Session);
	TArray<TSharedPtr<FJsonValue>> Events;
	int32 Remaining = Ctx.Limit;
	const bool bOk = Ctx.AnimProvider->ReadAnimSequencePlayersTimeline(ObjectId, [&](const IAnimationProvider::AnimSequencePlayersTimeline& Timeline)
	{
		Timeline.EnumerateEvents(Ctx.StartTime, Ctx.EndTime,
			[&](double StartTime, double /*EndTime*/, uint32 /*Depth*/, const FAnimSequencePlayerMessage& Message)
			{
				if (Remaining <= 0) return TraceServices::EEventEnumerate::Stop;
				TSharedPtr<FJsonObject> Row = MakeShared<FJsonObject>();
				Row->SetNumberField(TEXT("node_id"), Message.NodeId);
				Row->SetNumberField(TEXT("position"), Message.Position);
				Row->SetNumberField(TEXT("length"), Message.Length);
				Row->SetNumberField(TEXT("frame_counter"), Message.FrameCounter);
				Row->SetNumberField(TEXT("time"), StartTime);
				Events.Add(MakeShared<FJsonValueObject>(Row));
				--Remaining;
				return TraceServices::EEventEnumerate::Continue;
			});
	});

	TSharedPtr<FJsonObject> J = MakeShared<FJsonObject>();
	J->SetStringField(TEXT("object_id"), LexToString(ObjectId));
	J->SetBoolField(TEXT("has_timeline"), bOk);
	J->SetArrayField(TEXT("sequence_players"), Events);
	J->SetNumberField(TEXT("count"), Events.Num());
	J->SetObjectField(TEXT("sample"), SessionMeta(Ctx));
	return FAxonActionResult::Success(J);
}

FAxonActionResult FAxonRewindDebuggerActions::HandleSampleSkeletalPose(const TSharedPtr<FJsonObject>& Params)
{
	uint64 ObjectId = 0; FString IdErr;
	if (!ParseObjectId(Params, ObjectId, IdErr)) return FAxonActionResult::Error(IdErr, FAxonJsonUtils::ErrInvalidParams);

	FSampleContext Ctx;
	if (FString Err = ResolveSampleContext(Params, Ctx); !Err.IsEmpty()) return FAxonActionResult::Error(Err);
	if (FAxonActionResult E = NeedAnim(Ctx); !E.bSuccess) return E;

	const bool bFullPose = ParseBool(Params, TEXT("include_full_pose"), false);
	const int32 BoneLimit = FMath::Clamp(ParseLimit(Params, MaxPoseBones), 1, MaxPoseBones);

	TraceServices::FAnalysisSessionReadScope SessionReadScope(*Ctx.Session);

	TSharedPtr<FJsonObject> J = MakeShared<FJsonObject>();
	J->SetStringField(TEXT("object_id"), LexToString(ObjectId));
	J->SetStringField(TEXT("mode"), bFullPose ? TEXT("full") : TEXT("summary"));
	J->SetObjectField(TEXT("sample"), SessionMeta(Ctx));

	bool bFound = false;
	bool bHasMeshes = false;
	Ctx.AnimProvider->ReadSkeletalMeshPoseTimeline(ObjectId, [&](const IAnimationProvider::SkeletalMeshPoseTimeline& Timeline, bool bHasCurves)
	{
		bHasMeshes = true;
		J->SetBoolField(TEXT("has_curves"), bHasCurves);
		Timeline.EnumerateEvents(Ctx.StartTime, Ctx.EndTime,
			[&](double StartTime, double /*EndTime*/, uint32 /*Depth*/, const FSkeletalMeshPoseMessage& Message)
			{
				if (bFound) return TraceServices::EEventEnumerate::Stop;
				bFound = true;

				J->SetStringField(TEXT("mesh_id"), LexToString(Message.MeshId));
				J->SetStringField(TEXT("mesh_name"), Message.MeshName ? Message.MeshName : TEXT(""));
				J->SetNumberField(TEXT("num_transforms"), Message.NumTransforms);
				J->SetNumberField(TEXT("num_curves"), Message.NumCurves);
				J->SetNumberField(TEXT("lod_index"), Message.LodIndex);
				J->SetNumberField(TEXT("frame_counter"), Message.FrameCounter);
				J->SetBoolField(TEXT("is_visible"), Message.bIsVisible);
				J->SetNumberField(TEXT("recording_time"), Message.RecordingTime);
				J->SetNumberField(TEXT("time"), StartTime);
				J->SetObjectField(TEXT("component_to_world"), MakeTransformJson(Message.ComponentToWorld));

				const FSkeletalMeshInfo* MeshInfo = Ctx.AnimProvider->FindSkeletalMeshInfo(Message.MeshId);
				if (MeshInfo)
				{
					J->SetNumberField(TEXT("bone_count"), static_cast<double>(MeshInfo->BoneCount));
					J->SetStringField(TEXT("skeleton_id"), LexToString(MeshInfo->SkeletonId));
				}

				if (bFullPose && MeshInfo)
				{
					FTransform ComponentToWorld;
					TArray<FTransform> Bones;
					Ctx.AnimProvider->GetSkeletalMeshComponentSpacePose(Message, *MeshInfo, ComponentToWorld, Bones);
					J->SetObjectField(TEXT("resolved_component_to_world"), MakeTransformJson(ComponentToWorld));

					TArray<TSharedPtr<FJsonValue>> BoneArr;
					const int32 Count = FMath::Min(Bones.Num(), BoneLimit);
					J->SetNumberField(TEXT("bones_returned"), Count);
					J->SetNumberField(TEXT("bones_truncated"), Bones.Num() > BoneLimit);
					J->SetNumberField(TEXT("bone_hard_limit"), MaxPoseBones);
					for (int32 i = 0; i < Count; ++i)
					{
						TSharedPtr<FJsonObject> Bone = MakeTransformJson(Bones[i]);
						Bone->SetNumberField(TEXT("bone_index"), i);
						BoneArr.Add(MakeShared<FJsonValueObject>(Bone));
					}
					J->SetArrayField(TEXT("bones"), BoneArr);
				}
				return TraceServices::EEventEnumerate::Stop;
			});
	});

	J->SetBoolField(TEXT("has_timeline"), bHasMeshes);
	J->SetBoolField(TEXT("found_pose"), bFound);
	return FAxonActionResult::Success(J);
}

FAxonActionResult FAxonRewindDebuggerActions::HandleSampleCameraGraphResult(const TSharedPtr<FJsonObject>& Params)
{
#if !WITH_CAMERA_BLUEPRINT
	return FAxonActionResult::Error(
		TEXT("CameraBlueprint plugin not enabled — sample_camera_graph_result unavailable"),
		FAxonJsonUtils::ErrOptionalDepUnavailable);
#else
	uint64 ObjectId = 0; FString IdErr;
	if (!ParseObjectId(Params, ObjectId, IdErr)) return FAxonActionResult::Error(IdErr, FAxonJsonUtils::ErrInvalidParams);

	FSampleContext Ctx;
	if (FString Err = ResolveSampleContext(Params, Ctx); !Err.IsEmpty()) return FAxonActionResult::Error(Err);

	TraceServices::FAnalysisSessionReadScope SessionReadScope(*Ctx.Session);
	const ICameraTraceProvider* CameraProvider = Ctx.Session->ReadProvider<ICameraTraceProvider>(TEXT("CameraTraceProvider"));
	if (!CameraProvider)
	{
		return FAxonActionResult::Error(TEXT("CameraTraceProvider not present in analysis session (was CameraBlueprintChannel enabled while recording?)"));
	}

	TArray<TSharedPtr<FJsonValue>> Events;
	int32 Remaining = Ctx.Limit;
	const bool bOk = CameraProvider->ReadCameraGraphResultsTimeline(ObjectId, [&](const ICameraTraceProvider::CameraGraphResultsTimeline& Timeline)
	{
		Timeline.EnumerateEvents(Ctx.StartTime, Ctx.EndTime,
			[&](double StartTime, double /*EndTime*/, uint32 /*Depth*/, const FCameraGraphResultMessage& Message)
			{
				if (Remaining <= 0) return TraceServices::EEventEnumerate::Stop;
				TSharedPtr<FJsonObject> Row = MakeShared<FJsonObject>();
				Row->SetNumberField(TEXT("time"), StartTime);
				Row->SetNumberField(TEXT("recording_time"), Message.RecordingTime);
				Row->SetNumberField(TEXT("fov"), Message.FOV);
				Row->SetNumberField(TEXT("arm_length"), Message.ArmLength);
				auto Vec = [](const FVector& V)
				{
					TSharedPtr<FJsonObject> O = MakeShared<FJsonObject>();
					O->SetNumberField(TEXT("x"), V.X); O->SetNumberField(TEXT("y"), V.Y); O->SetNumberField(TEXT("z"), V.Z);
					return O;
				};
				auto Rot = [](const FRotator& R)
				{
					TSharedPtr<FJsonObject> O = MakeShared<FJsonObject>();
					O->SetNumberField(TEXT("pitch"), R.Pitch); O->SetNumberField(TEXT("yaw"), R.Yaw); O->SetNumberField(TEXT("roll"), R.Roll);
					return O;
				};
				Row->SetObjectField(TEXT("view_target_location"), Vec(Message.ViewTargetLocation));
				Row->SetObjectField(TEXT("view_target_rotation"), Rot(Message.ViewTargetRotation));
				Row->SetObjectField(TEXT("camera_rotation"), Rot(Message.CameraRotation));
				Row->SetObjectField(TEXT("camera_location_offset"), Vec(Message.CameraLocationOffset));
				Row->SetObjectField(TEXT("viewpoint_location_offset"), Vec(Message.ViewPointLocationOffset));
				Row->SetObjectField(TEXT("viewpoint_lag"), Vec(Message.ViewPointLag));
				Events.Add(MakeShared<FJsonValueObject>(Row));
				--Remaining;
				return TraceServices::EEventEnumerate::Continue;
			});
	});

	TSharedPtr<FJsonObject> J = MakeShared<FJsonObject>();
	J->SetStringField(TEXT("object_id"), LexToString(ObjectId));
	J->SetBoolField(TEXT("has_timeline"), bOk);
	J->SetArrayField(TEXT("results"), Events);
	J->SetNumberField(TEXT("count"), Events.Num());
	J->SetObjectField(TEXT("sample"), SessionMeta(Ctx));
	return FAxonActionResult::Success(J);
#endif
}

FAxonActionResult FAxonRewindDebuggerActions::HandleSampleCameraWatches(const TSharedPtr<FJsonObject>& Params)
{
#if !WITH_CAMERA_BLUEPRINT
	return FAxonActionResult::Error(
		TEXT("CameraBlueprint plugin not enabled — sample_camera_watches unavailable"),
		FAxonJsonUtils::ErrOptionalDepUnavailable);
#else
	uint64 ObjectId = 0; FString IdErr;
	if (!ParseObjectId(Params, ObjectId, IdErr)) return FAxonActionResult::Error(IdErr, FAxonJsonUtils::ErrInvalidParams);

	FSampleContext Ctx;
	if (FString Err = ResolveSampleContext(Params, Ctx); !Err.IsEmpty()) return FAxonActionResult::Error(Err);

	TraceServices::FAnalysisSessionReadScope SessionReadScope(*Ctx.Session);
	const ICameraTraceProvider* CameraProvider = Ctx.Session->ReadProvider<ICameraTraceProvider>(TEXT("CameraTraceProvider"));
	if (!CameraProvider)
	{
		return FAxonActionResult::Error(TEXT("CameraTraceProvider not present in analysis session"));
	}

	TArray<TSharedPtr<FJsonValue>> Events;
	int32 Remaining = Ctx.Limit;
	const bool bOk = CameraProvider->ReadCameraWatchesTimeline(ObjectId, [&](const ICameraTraceProvider::CameraWatchesTimeline& Timeline)
	{
		Timeline.EnumerateEvents(Ctx.StartTime, Ctx.EndTime,
			[&](double StartTime, double /*EndTime*/, uint32 /*Depth*/, const FCameraWatchMessage& Message)
			{
				if (Remaining <= 0) return TraceServices::EEventEnumerate::Stop;
				TSharedPtr<FJsonObject> Row = MakeShared<FJsonObject>();
				Row->SetNumberField(TEXT("node_id"), Message.NodeId);
				Row->SetNumberField(TEXT("fov"), Message.FOV);
				Row->SetNumberField(TEXT("time"), StartTime);
				TSharedPtr<FJsonObject> Loc = MakeShared<FJsonObject>();
				Loc->SetNumberField(TEXT("x"), Message.Location.X);
				Loc->SetNumberField(TEXT("y"), Message.Location.Y);
				Loc->SetNumberField(TEXT("z"), Message.Location.Z);
				Row->SetObjectField(TEXT("location"), Loc);
				TSharedPtr<FJsonObject> Rot = MakeShared<FJsonObject>();
				Rot->SetNumberField(TEXT("pitch"), Message.Rotation.Pitch);
				Rot->SetNumberField(TEXT("yaw"), Message.Rotation.Yaw);
				Rot->SetNumberField(TEXT("roll"), Message.Rotation.Roll);
				Row->SetObjectField(TEXT("rotation"), Rot);
				Events.Add(MakeShared<FJsonValueObject>(Row));
				--Remaining;
				return TraceServices::EEventEnumerate::Continue;
			});
	});

	TSharedPtr<FJsonObject> J = MakeShared<FJsonObject>();
	J->SetStringField(TEXT("object_id"), LexToString(ObjectId));
	J->SetBoolField(TEXT("has_timeline"), bOk);
	J->SetArrayField(TEXT("watches"), Events);
	J->SetNumberField(TEXT("count"), Events.Num());
	J->SetObjectField(TEXT("sample"), SessionMeta(Ctx));
	return FAxonActionResult::Success(J);
#endif
}

FAxonActionResult FAxonRewindDebuggerActions::HandleResolveObject(const TSharedPtr<FJsonObject>& Params)
{
	uint64 ObjectId = 0; FString IdErr;
	if (!ParseObjectId(Params, ObjectId, IdErr)) return FAxonActionResult::Error(IdErr, FAxonJsonUtils::ErrInvalidParams);

	FSampleContext Ctx;
	if (FString Err = ResolveSampleContext(Params, Ctx, false); !Err.IsEmpty()) return FAxonActionResult::Error(Err);
	if (!Ctx.GameplayProvider)
	{
		return FAxonActionResult::Error(TEXT("GameplayProvider not present in analysis session"));
	}

	TraceServices::FAnalysisSessionReadScope SessionReadScope(*Ctx.Session);
	if (!Ctx.GameplayProvider->FindObjectInfo(ObjectId))
	{
		return FAxonActionResult::Error(FString::Printf(TEXT("Object id %s not found in session"), *LexToString(ObjectId)));
	}

	return FAxonActionResult::Success(MakeObjectInfoJson(Ctx.GameplayProvider, ObjectId));
}
