#include "AxonRewindDebuggerUtils.h"
#include "AxonJsonUtils.h"

namespace AxonRewindDebugger
{
	static bool ResolveProfileTimeFromRecording(const IGameplayProvider* GameplayProvider, double RecordingTime, double& OutProfileTime)
	{
		if (!GameplayProvider)
		{
			return false;
		}

		// Recording indices typically start at 1 after StartRecording; probe a small range.
		for (uint32 RecordingId = 0; RecordingId < 16; ++RecordingId)
		{
			const IGameplayProvider::RecordingInfoTimeline* Recording = GameplayProvider->GetRecordingInfo(RecordingId);
			if (!Recording || Recording->GetEventCount() == 0)
			{
				continue;
			}

			const uint64 EventCount = Recording->GetEventCount();
			const FRecordingInfoMessage& First = Recording->GetEvent(0);
			const FRecordingInfoMessage& Last = Recording->GetEvent(EventCount - 1);
			if (RecordingTime <= First.ElapsedTime)
			{
				OutProfileTime = First.ProfileTime;
				return true;
			}
			if (RecordingTime >= Last.ElapsedTime)
			{
				OutProfileTime = Last.ProfileTime;
				return true;
			}

			for (uint64 i = 1; i < EventCount; ++i)
			{
				const FRecordingInfoMessage& Prev = Recording->GetEvent(i - 1);
				const FRecordingInfoMessage& Cur = Recording->GetEvent(i);
				if (Prev.ElapsedTime <= RecordingTime && Cur.ElapsedTime >= RecordingTime)
				{
					OutProfileTime = (RecordingTime - Prev.ElapsedTime <= Cur.ElapsedTime - RecordingTime)
						? Prev.ProfileTime : Cur.ProfileTime;
					return true;
				}
			}
		}
		return false;
	}

	bool ParseObjectId(const TSharedPtr<FJsonObject>& Params, uint64& OutId, FString& OutError, bool bRequired)
	{
		OutId = 0;
		FString IdStr;
		if (!Params.IsValid() || !Params->TryGetStringField(TEXT("object_id"), IdStr) || IdStr.IsEmpty())
		{
			if (bRequired)
			{
				OutError = TEXT("object_id (uint64 string) is required");
				return false;
			}
			return true;
		}

		LexFromString(OutId, *IdStr);
		if (OutId == 0)
		{
			double Num = 0.0;
			if (Params->TryGetNumberField(TEXT("object_id"), Num) && Num > 0.0)
			{
				OutId = static_cast<uint64>(Num);
			}
		}
		if (OutId == 0)
		{
			OutError = TEXT("object_id must be a non-zero uint64 string");
			return false;
		}
		return true;
	}

	int32 ParseLimit(const TSharedPtr<FJsonObject>& Params, int32 DefaultValue)
	{
		double Num = DefaultValue;
		if (Params.IsValid())
		{
			Params->TryGetNumberField(TEXT("limit"), Num);
		}
		return FMath::Clamp(static_cast<int32>(Num), 1, 1000);
	}

	bool ParseBool(const TSharedPtr<FJsonObject>& Params, const TCHAR* Field, bool DefaultValue)
	{
		bool Value = DefaultValue;
		if (Params.IsValid())
		{
			Params->TryGetBoolField(Field, Value);
		}
		return Value;
	}

	FString ResolveSampleContext(const TSharedPtr<FJsonObject>& Params, FSampleContext& Out, bool bAllowScrubWrite)
	{
		Out = FSampleContext();
		Out.Debugger = IRewindDebugger::Instance();
		if (!Out.Debugger)
		{
			return TEXT("Rewind Debugger is not available (GameplayInsights / RewindDebugger not loaded)");
		}

		Out.Session = Out.Debugger->GetAnalysisSession();
		if (!Out.Session)
		{
			return TEXT("No analysis session — start or load a Rewind Debugger recording first");
		}

		Out.Limit = ParseLimit(Params);
		Out.ScrubTime = Out.Debugger->GetScrubTime();
		Out.TraceTime = Out.Debugger->CurrentTraceTime();

		TraceServices::FAnalysisSessionReadScope SessionReadScope(*Out.Session);
		Out.AnimProvider = Out.Session->ReadProvider<IAnimationProvider>(TEXT("AnimationProvider"));
		Out.GameplayProvider = Out.Session->ReadProvider<IGameplayProvider>(TEXT("GameplayProvider"));

		double TimeOverride = 0.0;
		const bool bHasTime = Params.IsValid() && Params->TryGetNumberField(TEXT("time"), TimeOverride);
		if (bHasTime)
		{
			Out.ScrubTime = TimeOverride;
			double ProfileTime = Out.TraceTime;
			if (ResolveProfileTimeFromRecording(Out.GameplayProvider, TimeOverride, ProfileTime))
			{
				Out.TraceTime = ProfileTime;
			}
			// Note: IRewindDebugger has no public ScrubToTime; UI scrub is not moved.
			Out.bMovedScrub = false;
			(void)bAllowScrubWrite;
		}

		const TraceServices::IFrameProvider& FrameProvider = TraceServices::ReadFrameProvider(*Out.Session);
		TraceServices::FFrame Frame;
		if (FrameProvider.GetFrameFromTime(TraceFrameType_Game, Out.TraceTime, Frame))
		{
			Out.StartTime = Frame.StartTime;
			Out.EndTime = Frame.EndTime;
		}
		else
		{
			Out.StartTime = Out.TraceTime;
			Out.EndTime = Out.TraceTime + 0.001;
		}

		double FrameWindow = 0.0;
		if (Params.IsValid() && Params->TryGetNumberField(TEXT("frame_window"), FrameWindow) && FrameWindow > 0.0)
		{
			Out.StartTime = Out.TraceTime - FrameWindow;
			Out.EndTime = Out.TraceTime + FrameWindow;
		}

		return FString();
	}

	TSharedPtr<FJsonObject> MakeObjectInfoJson(const IGameplayProvider* Provider, uint64 ObjectId)
	{
		TSharedPtr<FJsonObject> J = MakeShared<FJsonObject>();
		J->SetStringField(TEXT("object_id"), LexToString(ObjectId));
		if (!Provider)
		{
			return J;
		}

		if (const FObjectInfo* Info = Provider->FindObjectInfo(ObjectId))
		{
			J->SetStringField(TEXT("name"), Info->Name ? Info->Name : TEXT(""));
			J->SetStringField(TEXT("path_name"), Info->PathName ? Info->PathName : TEXT(""));
			J->SetStringField(TEXT("outer_object_id"), LexToString(Info->GetOuterUObjectId()));
			J->SetNumberField(TEXT("class_id"), static_cast<double>(Info->ClassId));
			if (const FClassInfo* ClassInfo = Provider->FindClassInfo(Info->ClassId))
			{
				J->SetStringField(TEXT("class_name"), ClassInfo->Name ? ClassInfo->Name : TEXT(""));
				J->SetStringField(TEXT("class_path"), ClassInfo->PathName ? ClassInfo->PathName : TEXT(""));
			}
		}
		return J;
	}

	TSharedPtr<FJsonObject> MakeTransformJson(const FTransform& Xform)
	{
		TSharedPtr<FJsonObject> J = MakeShared<FJsonObject>();
		const FVector Loc = Xform.GetLocation();
		const FRotator Rot = Xform.Rotator();
		const FVector Scale = Xform.GetScale3D();
		auto Vec = [](const FVector& V)
		{
			TSharedPtr<FJsonObject> O = MakeShared<FJsonObject>();
			O->SetNumberField(TEXT("x"), V.X);
			O->SetNumberField(TEXT("y"), V.Y);
			O->SetNumberField(TEXT("z"), V.Z);
			return O;
		};
		auto RotJ = [](const FRotator& R)
		{
			TSharedPtr<FJsonObject> O = MakeShared<FJsonObject>();
			O->SetNumberField(TEXT("pitch"), R.Pitch);
			O->SetNumberField(TEXT("yaw"), R.Yaw);
			O->SetNumberField(TEXT("roll"), R.Roll);
			return O;
		};
		J->SetObjectField(TEXT("location"), Vec(Loc));
		J->SetObjectField(TEXT("rotation"), RotJ(Rot));
		J->SetObjectField(TEXT("scale"), Vec(Scale));
		return J;
	}

	TSharedPtr<FJsonObject> MakeVariantValueJson(const FVariantValue& Value, const IAnimationProvider* /*AnimProvider*/)
	{
		TSharedPtr<FJsonObject> J = MakeShared<FJsonObject>();
		switch (Value.Type)
		{
		case EAnimNodeValueType::Bool:
			J->SetStringField(TEXT("type"), TEXT("bool"));
			J->SetBoolField(TEXT("value"), Value.Bool.bValue);
			break;
		case EAnimNodeValueType::Int32:
			J->SetStringField(TEXT("type"), TEXT("int32"));
			J->SetNumberField(TEXT("value"), Value.Int32.Value);
			break;
		case EAnimNodeValueType::Float:
			J->SetStringField(TEXT("type"), TEXT("float"));
			J->SetNumberField(TEXT("value"), Value.Float.Value);
			break;
		case EAnimNodeValueType::Vector2D:
			J->SetStringField(TEXT("type"), TEXT("vector2d"));
			J->SetNumberField(TEXT("x"), Value.Vector2D.Value.X);
			J->SetNumberField(TEXT("y"), Value.Vector2D.Value.Y);
			break;
		case EAnimNodeValueType::Vector:
			J->SetStringField(TEXT("type"), TEXT("vector"));
			J->SetNumberField(TEXT("x"), Value.Vector.Value.X);
			J->SetNumberField(TEXT("y"), Value.Vector.Value.Y);
			J->SetNumberField(TEXT("z"), Value.Vector.Value.Z);
			break;
		case EAnimNodeValueType::String:
			J->SetStringField(TEXT("type"), TEXT("string"));
			J->SetStringField(TEXT("value"), Value.String.Value ? Value.String.Value : TEXT(""));
			break;
		case EAnimNodeValueType::Object:
			J->SetStringField(TEXT("type"), TEXT("object"));
			J->SetStringField(TEXT("object_id"), LexToString(Value.Object.Value));
			J->SetNumberField(TEXT("playback_time"), Value.Object.PlaybackTime);
			J->SetNumberField(TEXT("blend_x"), Value.Object.BlendX);
			J->SetNumberField(TEXT("blend_y"), Value.Object.BlendY);
			break;
		case EAnimNodeValueType::Class:
			J->SetStringField(TEXT("type"), TEXT("class"));
			J->SetStringField(TEXT("class_id"), LexToString(Value.Class.Value));
			break;
		case EAnimNodeValueType::AnimNode:
			J->SetStringField(TEXT("type"), TEXT("anim_node"));
			J->SetNumberField(TEXT("node_id"), Value.AnimNode.Value);
			J->SetStringField(TEXT("anim_instance_id"), LexToString(Value.AnimNode.AnimInstanceId));
			break;
		default:
			J->SetStringField(TEXT("type"), TEXT("unknown"));
			break;
		}
		return J;
	}

	void AppendDebugObjectTree(const TSharedPtr<FDebugObjectInfo>& Info, TArray<TSharedPtr<FJsonValue>>& Out, int32 Depth, int32& Remaining)
	{
		if (!Info.IsValid() || Remaining <= 0)
		{
			return;
		}

		TSharedPtr<FJsonObject> J = MakeShared<FJsonObject>();
		J->SetStringField(TEXT("object_id"), LexToString(Info->GetUObjectId()));
		J->SetStringField(TEXT("name"), Info->ObjectName);
		J->SetBoolField(TEXT("expanded"), Info->bExpanded);
		J->SetNumberField(TEXT("depth"), Depth);
		Out.Add(MakeShared<FJsonValueObject>(J));
		--Remaining;

		for (const TSharedPtr<FDebugObjectInfo>& Child : Info->Children)
		{
			AppendDebugObjectTree(Child, Out, Depth + 1, Remaining);
		}
	}
}