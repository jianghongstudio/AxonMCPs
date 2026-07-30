#include "AxonPieSession.h"

#include "AxonPieObject.h"

#include "Editor.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Components/ActorComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Misc/DateTime.h"

DEFINE_LOG_CATEGORY_STATIC(LogAxonPieSession, Log, All);

namespace
{
	const TCHAR* StatusToString(EAxonPieSessionStatus Status)
	{
		switch (Status)
		{
		case EAxonPieSessionStatus::Running: return TEXT("running");
		case EAxonPieSessionStatus::Complete: return TEXT("complete");
		case EAxonPieSessionStatus::Stopped: return TEXT("stopped");
		default: return TEXT("unknown");
		}
	}

	UObject* ResolveTarget(UWorld* PieWorld, const FAxonPieSession& Session)
	{
		if (!PieWorld)
		{
			return nullptr;
		}

		AActor* Actor = nullptr;
		for (TActorIterator<AActor> It(PieWorld); It; ++It)
		{
			AActor* Candidate = *It;
			if (!Candidate)
			{
				continue;
			}
#if WITH_EDITOR
			const FString Label = Candidate->GetActorLabel();
#else
			const FString Label;
#endif
			const FString Name = Candidate->GetName();
			const FString ClassName = Candidate->GetClass() ? Candidate->GetClass()->GetName() : FString();
			if (!Session.TargetActorLabel.IsEmpty() && Label == Session.TargetActorLabel)
			{
				Actor = Candidate;
				break;
			}
			if (!Session.TargetObjectName.IsEmpty() && Name == Session.TargetObjectName)
			{
				Actor = Candidate;
				break;
			}
			if (!Session.TargetClassName.IsEmpty() && ClassName.Contains(Session.TargetClassName))
			{
				Actor = Candidate;
				break;
			}
		}

		if (!Actor)
		{
			return nullptr;
		}
		if (Session.bTargetAnimInstance)
		{
			for (UActorComponent* Component : Actor->GetComponents())
			{
				USkeletalMeshComponent* MeshComponent = Cast<USkeletalMeshComponent>(Component);
				if (MeshComponent && (Session.TargetComponentName.IsEmpty() || MeshComponent->GetName() == Session.TargetComponentName))
				{
					return MeshComponent->GetAnimInstance();
				}
			}
			return nullptr;
		}
		if (!Session.TargetComponentName.IsEmpty())
		{
			for (UActorComponent* Component : Actor->GetComponents())
			{
				if (Component && Component->GetName() == Session.TargetComponentName)
				{
					return Component;
				}
			}
			return nullptr;
		}
		return Actor;
	}

	APawn* ResolveProvocationPawn(UWorld* PieWorld, UObject* Target)
	{
		if (APawn* Pawn = Cast<APawn>(Target))
		{
			return Pawn;
		}
		if (PieWorld)
		{
			if (APlayerController* PlayerController = PieWorld->GetFirstPlayerController())
			{
				return PlayerController->GetPawn();
			}
		}
		return nullptr;
	}

	void FireProvocation(FAxonPieProvocation& Provocation, UWorld* PieWorld, UObject* Target, double ElapsedSeconds)
	{
		Provocation.bFired = true;
		Provocation.FiredAtSeconds = ElapsedSeconds;

		switch (Provocation.Action)
		{
		case EAxonPieProvocationAction::SetControlRotation:
			if (APlayerController* PlayerController = PieWorld ? PieWorld->GetFirstPlayerController() : nullptr)
			{
				PlayerController->SetControlRotation(Provocation.Rotation);
				Provocation.bDispatched = true;
				Provocation.Result = TEXT("set_control_rotation");
			}
			else
			{
				Provocation.Result = TEXT("no_player_controller");
			}
			break;

		case EAxonPieProvocationAction::AddMovementInput:
			if (APawn* Pawn = ResolveProvocationPawn(PieWorld, Target))
			{
				Pawn->AddMovementInput(Provocation.Direction, static_cast<float>(Provocation.Scale));
				Provocation.bDispatched = true;
				Provocation.Result = TEXT("add_movement_input");
			}
			else
			{
				Provocation.Result = TEXT("no_pawn");
			}
			break;

		case EAxonPieProvocationAction::Jump:
			if (ACharacter* Character = Cast<ACharacter>(ResolveProvocationPawn(PieWorld, Target)))
			{
				Character->Jump();
				Provocation.bDispatched = true;
				Provocation.Result = TEXT("jump");
			}
			else
			{
				Provocation.Result = TEXT("pawn_not_a_character");
			}
			break;

		case EAxonPieProvocationAction::ConsoleCommand:
			if (Provocation.Command.IsEmpty())
			{
				Provocation.Result = TEXT("empty_command");
			}
			else if (APlayerController* PlayerController = PieWorld ? PieWorld->GetFirstPlayerController() : nullptr)
			{
				PlayerController->ConsoleCommand(Provocation.Command, true);
				Provocation.bDispatched = true;
				Provocation.Result = TEXT("console_command");
			}
			else if (GEngine && PieWorld)
			{
				GEngine->Exec(PieWorld, *Provocation.Command);
				Provocation.bDispatched = true;
				Provocation.Result = TEXT("console_command");
			}
			else
			{
				Provocation.Result = TEXT("no_exec_target");
			}
			break;

		default:
			Provocation.Result = TEXT("unknown_action");
			break;
		}
	}
}

FAxonPieSessionManager& FAxonPieSessionManager::Get()
{
	static FAxonPieSessionManager Instance;
	return Instance;
}

FString FAxonPieSessionManager::CreateSession(FAxonPieSession&& Session)
{
	if (Session.Id.IsEmpty())
	{
		Session.Id = FString::Printf(TEXT("axon_pie_%u_%s"), NextSessionSerial++,
			*FDateTime::Now().ToString(TEXT("%H%M%S")));
	}
	const FString Id = Session.Id;
	Sessions.Add(Id, MoveTemp(Session));
	EnsureObserver();
	return Id;
}

FAxonPieSession* FAxonPieSessionManager::Find(const FString& SessionId)
{
	return Sessions.Find(SessionId);
}

int32 FAxonPieSessionManager::Stop(const FString& SessionId)
{
	int32 Stopped = 0;
	for (TPair<FString, FAxonPieSession>& Pair : Sessions)
	{
		if ((SessionId.IsEmpty() || Pair.Key == SessionId) && Pair.Value.Status == EAxonPieSessionStatus::Running)
		{
			Pair.Value.Status = EAxonPieSessionStatus::Stopped;
			Pair.Value.bStoppedByTool = true;
			Pair.Value.LastObservedSeconds = FPlatformTime::Seconds();
			++Stopped;
		}
	}

	if (!HasRunningSessions() && GEditor && AxonPieObject::FindPieWorld())
	{
		GEditor->RequestEndPlayMap();
		for (TPair<FString, FAxonPieSession>& Pair : Sessions)
		{
			Pair.Value.bTeardownStarted = true;
		}
	}
	return Stopped;
}

bool FAxonPieSessionManager::HasRunningSessions() const
{
	for (const TPair<FString, FAxonPieSession>& Pair : Sessions)
	{
		if (Pair.Value.Status == EAxonPieSessionStatus::Running)
		{
			return true;
		}
	}
	return false;
}

TSharedPtr<FJsonObject> FAxonPieSessionManager::BuildReport(const FAxonPieSession& Session, bool bIncludeSamples) const
{
	TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();
	Root->SetStringField(TEXT("session_id"), Session.Id);
	Root->SetStringField(TEXT("status"), StatusToString(Session.Status));
	Root->SetStringField(TEXT("map"), Session.MapName);
	Root->SetNumberField(TEXT("duration"), Session.DurationSeconds);
	Root->SetBoolField(TEXT("pie_active"), Session.bPieActive);
	Root->SetBoolField(TEXT("pie_ready"), Session.bReady);
	Root->SetNumberField(TEXT("timeseries_sample_count"), Session.Samples.Num());
	const double EndTime = Session.Status == EAxonPieSessionStatus::Running
		? FPlatformTime::Seconds() : Session.LastObservedSeconds;
	Root->SetNumberField(TEXT("elapsed_seconds"), FMath::Max(0.0, EndTime - Session.StartTimeSeconds));

	TArray<TSharedPtr<FJsonValue>> Provocations;
	for (const FAxonPieProvocation& Provocation : Session.Provocations)
	{
		TSharedPtr<FJsonObject> Item = MakeShared<FJsonObject>();
		Item->SetStringField(TEXT("action"), Provocation.RawAction);
		Item->SetNumberField(TEXT("time"), Provocation.AtSeconds);
		Item->SetBoolField(TEXT("fired"), Provocation.bFired);
		Item->SetNumberField(TEXT("fired_at_seconds"), Provocation.FiredAtSeconds);
		Item->SetBoolField(TEXT("dispatched"), Provocation.bDispatched);
		if (!Provocation.Result.IsEmpty())
		{
			Item->SetStringField(TEXT("result"), Provocation.Result);
		}
		Provocations.Add(MakeShared<FJsonValueObject>(Item));
	}
	Root->SetArrayField(TEXT("provocations"), Provocations);

	if (bIncludeSamples)
	{
		TArray<TSharedPtr<FJsonValue>> Series;
		for (const FAxonPieTimeseriesSample& Sample : Session.Samples)
		{
			TSharedPtr<FJsonObject> Item = MakeShared<FJsonObject>();
			Item->SetNumberField(TEXT("t"), Sample.TimeSeconds);
			TSharedPtr<FJsonObject> Variables = MakeShared<FJsonObject>();
			for (const TPair<FString, TSharedPtr<FJsonValue>>& Variable : Sample.Vars)
			{
				if (Variable.Value.IsValid())
				{
					Variables->SetField(Variable.Key, Variable.Value);
				}
			}
			Item->SetObjectField(TEXT("vars"), Variables);
			Series.Add(MakeShared<FJsonValueObject>(Item));
		}
		Root->SetArrayField(TEXT("timeseries"), Series);
	}
	return Root;
}

void FAxonPieSessionManager::EnsureObserver()
{
	if (bObserverActive)
	{
		return;
	}
	bObserverActive = true;
	TickerHandle = FTSTicker::GetCoreTicker().AddTicker(
		TEXT("AxonPieTimeseriesObserver"), 0.0f,
		[](float DeltaTime) { return FAxonPieSessionManager::Get().OnFrameTick(DeltaTime); });
	EndPieHandle = FEditorDelegates::EndPIE.AddRaw(this, &FAxonPieSessionManager::OnPieEnded);
	PrePieEndedHandle = FEditorDelegates::PrePIEEnded.AddRaw(this, &FAxonPieSessionManager::OnPieEnded);
}

void FAxonPieSessionManager::TeardownObserverIfIdle()
{
	if (HasRunningSessions() || !bObserverActive)
	{
		return;
	}
	bObserverActive = false;
	if (TickerHandle.IsValid())
	{
		FTSTicker::RemoveTicker(TickerHandle);
		TickerHandle.Reset();
	}
	if (EndPieHandle.IsValid())
	{
		FEditorDelegates::EndPIE.Remove(EndPieHandle);
		EndPieHandle.Reset();
	}
	if (PrePieEndedHandle.IsValid())
	{
		FEditorDelegates::PrePIEEnded.Remove(PrePieEndedHandle);
		PrePieEndedHandle.Reset();
	}
}

void FAxonPieSessionManager::OnPieEnded(bool)
{
	for (TPair<FString, FAxonPieSession>& Pair : Sessions)
	{
		Pair.Value.bPieActive = false;
	}
}

bool FAxonPieSessionManager::OnFrameTick(float)
{
	UWorld* PieWorld = AxonPieObject::FindPieWorld();
	for (TPair<FString, FAxonPieSession>& Pair : Sessions)
	{
		FAxonPieSession& Session = Pair.Value;
		if (Session.Status != EAxonPieSessionStatus::Running)
		{
			continue;
		}

		Session.LastObservedSeconds = FPlatformTime::Seconds();
		if (!Session.bPieActive || !PieWorld || !IsValid(PieWorld))
		{
			Session.Status = EAxonPieSessionStatus::Complete;
			continue;
		}
		if (!PieWorld->HasBegunPlay())
		{
			continue;
		}

		Session.bReady = true;
		AdvanceSession(Session);
		if (Session.LastObservedSeconds - Session.StartTimeSeconds >= Session.DurationSeconds)
		{
			Session.Status = EAxonPieSessionStatus::Complete;
		}
	}

	if (!HasRunningSessions())
	{
		if (GEditor && AxonPieObject::FindPieWorld())
		{
			GEditor->RequestEndPlayMap();
			for (TPair<FString, FAxonPieSession>& Pair : Sessions)
			{
				Pair.Value.bTeardownStarted = true;
			}
		}
		TeardownObserverIfIdle();
		return false;
	}
	return true;
}

void FAxonPieSessionManager::AdvanceSession(FAxonPieSession& Session)
{
	UWorld* PieWorld = AxonPieObject::FindPieWorld();
	if (!PieWorld)
	{
		return;
	}

	const double SampleTime = FPlatformTime::Seconds() - Session.StartTimeSeconds;
	UObject* Target = Session.CachedTarget.IsValid() ? Session.CachedTarget.Get() : nullptr;
	if (!Target)
	{
		Target = ResolveTarget(PieWorld, Session);
		Session.CachedTarget = Target;
	}

	for (FAxonPieProvocation& Provocation : Session.Provocations)
	{
		if (!Provocation.bFired && SampleTime >= Provocation.AtSeconds)
		{
			FireProvocation(Provocation, PieWorld, Target, SampleTime);
		}
	}

	const bool bSampleDue = Session.LastSampleSeconds < 0.0 ||
		SampleTime - Session.LastSampleSeconds >= Session.SampleInterval;
	if (!bSampleDue || Session.Samples.Num() >= Session.MaxSamples)
	{
		return;
	}

	FAxonPieTimeseriesSample Sample;
	Sample.TimeSeconds = SampleTime;
	if (Target)
	{
		for (const FString& Path : Session.VariablePaths)
		{
			FString TypeName;
			if (TSharedPtr<FJsonValue> Value = AxonPieObject::ReadDottedValue(Target, Path, TypeName))
			{
				Sample.Vars.Emplace(Path, Value);
			}
		}
	}
	Session.Samples.Add(MoveTemp(Sample));
	Session.LastSampleSeconds = SampleTime;
}
