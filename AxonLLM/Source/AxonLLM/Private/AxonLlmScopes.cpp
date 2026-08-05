#include "AxonLlmScopes.h"
#include "AxonLlmClient.h"
#include "AxonLlmUsage.h"
#include "AxonLlmJobQueue.h"

#include "Interfaces/IPluginManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Dom/JsonValue.h"

namespace AxonLlmScopesPrivate
{
	static const TCHAR* SummarizeSystem =
		TEXT("You are an Axon knowledge worker. Summarize Unreal/_raw JSON evidence for an Agent.\n")
		TEXT("Rules:\n")
		TEXT("- Use ONLY facts present in the evidence. Never invent asset paths or API names.\n")
		TEXT("- Prefer compact Markdown: bullets and small tables.\n")
		TEXT("- Cite evidence file paths that appear in the input headers.\n")
		TEXT("- If something is missing, say it is not in the extract.\n")
		TEXT("- Reply in the same language as Extra instructions when provided; otherwise Chinese.\n");

	static const TCHAR* DraftSystem =
		TEXT("You are an Axon project-distillation writer. Produce a Knowledge topic Markdown draft.\n")
		TEXT("Rules:\n")
		TEXT("- Depth bar: comparable to AxonGaspKB topic docs (clear system explanation + evidence).\n")
		TEXT("- Every non-trivial claim must reference a _raw/ evidence path from the input.\n")
		TEXT("- Do not invent assets. Mark gaps explicitly.\n")
		TEXT("- Structure with title, short role blurb, sections, and an asset/evidence map when useful.\n")
		TEXT("- Output Markdown only (no surrounding code fence).\n");

	static const TCHAR* LogSystem =
		TEXT("You are an Unreal/Axon triage assistant. Summarize logs or build errors for an Agent.\n")
		TEXT("Rules:\n")
		TEXT("- List top 3 likely root causes with short evidence quotes.\n")
		TEXT("- Suggest next Axon MCP actions as plain names only (do not claim you executed them).\n")
		TEXT("- Prefer Chinese unless the log is English-only and Extra instructions say otherwise.\n");

	bool IsPathUnderDir(const FString& FilePath, const FString& Dir)
	{
		const FString FullFile = FPaths::ConvertRelativePathToFull(FilePath);
		FString FullDir = FPaths::ConvertRelativePathToFull(Dir);
		if (!FullDir.EndsWith(TEXT("/")) && !FullDir.EndsWith(TEXT("\\")))
		{
			FullDir += TEXT("/");
		}
		return FullFile.StartsWith(FullDir);
	}

	TSharedPtr<FJsonObject> MakeRunBase(
		const FAxonLlmScopeRequest& Req,
		int32 CharsIn,
		int32 CharsOut,
		double LatencyMs,
		bool bTruncated,
		const TArray<FString>& UsedPaths)
	{
		TSharedPtr<FJsonObject> Out = MakeShared<FJsonObject>();
		Out->SetNumberField(TEXT("worker_index"), Req.WorkerIndex);
		Out->SetStringField(TEXT("model"), Req.Worker.Model);
		Out->SetStringField(TEXT("scope"), UAxonLLMSettings::ScopeToWire(Req.Scope));
		Out->SetNumberField(TEXT("chars_in"), CharsIn);
		Out->SetNumberField(TEXT("chars_out"), CharsOut);
		Out->SetNumberField(TEXT("latency_ms"), LatencyMs);
		Out->SetBoolField(TEXT("truncated"), bTruncated);
		TArray<TSharedPtr<FJsonValue>> PathVals;
		for (const FString& P : UsedPaths)
		{
			PathVals.Add(MakeShared<FJsonValueString>(P));
		}
		Out->SetArrayField(TEXT("evidence_paths"), PathVals);
		if (!Req.JobId.IsEmpty())
		{
			Out->SetStringField(TEXT("job_id"), Req.JobId);
		}
		return Out;
	}

	void RecordUsage(const FAxonLlmScopeRequest& Req, int32 Cin, int32 Cout, double Latency, bool bOk)
	{
		FAxonLlmUsage::Record(Req.Scope, Req.WorkerIndex, Req.Worker.Model, Cin, Cout, Latency, bOk, Req.JobId);
	}
}

FString FAxonLlmScopes::ResolveKnowledgeRoot(const FString& KbPlugin)
{
	FString Root;
	TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(KbPlugin);
	if (Plugin.IsValid())
	{
		Root = FPaths::Combine(Plugin->GetBaseDir(), TEXT("Knowledge"));
	}
	else
	{
		Root = FPaths::Combine(FPaths::ProjectPluginsDir(), TEXT("AxonMCPs"), KbPlugin, TEXT("Knowledge"));
	}
	return FPaths::ConvertRelativePathToFull(Root);
}

FString FAxonLlmScopes::ResolveRawRoot(const FString& KbPlugin)
{
	return FPaths::Combine(ResolveKnowledgeRoot(KbPlugin), TEXT("_raw"));
}

FString FAxonLlmScopes::ResolveDraftRoot(const FString& KbPlugin)
{
	return FPaths::Combine(ResolveKnowledgeRoot(KbPlugin), TEXT("_draft"));
}

bool FAxonLlmScopes::ScopeNeedsLlm(EAxonLlmScope Scope)
{
	return Scope != EAxonLlmScope::KnowledgePromoteDraft;
}

TArray<FString> FAxonLlmScopes::ParsePathsParam(const TSharedPtr<FJsonObject>& Params)
{
	TArray<FString> Paths;
	if (!Params.IsValid())
	{
		return Paths;
	}
	const TArray<TSharedPtr<FJsonValue>>* Arr = nullptr;
	if (Params->TryGetArrayField(TEXT("paths"), Arr) && Arr)
	{
		for (const TSharedPtr<FJsonValue>& V : *Arr)
		{
			if (V.IsValid() && V->Type == EJson::String)
			{
				const FString S = V->AsString().TrimStartAndEnd();
				if (!S.IsEmpty())
				{
					Paths.Add(S);
				}
			}
		}
		return Paths;
	}
	FString Single;
	if (Params->TryGetStringField(TEXT("paths"), Single) && !Single.IsEmpty())
	{
		Single.ParseIntoArray(Paths, TEXT(","), true);
		for (FString& P : Paths)
		{
			P.TrimStartAndEndInline();
		}
	}
	return Paths;
}

bool FAxonLlmScopes::ReadEvidenceFiles(
	const FString& KbPlugin,
	const TArray<FString>& RelativeOrAbsPaths,
	int32 MaxInputChars,
	FString& OutCombined,
	TArray<FString>& OutUsedPaths,
	bool& bOutTruncated,
	FString& OutError)
{
	OutCombined.Reset();
	OutUsedPaths.Reset();
	bOutTruncated = false;

	if (RelativeOrAbsPaths.Num() == 0)
	{
		OutError = TEXT("paths is required");
		return false;
	}

	const FString RawRoot = ResolveRawRoot(KbPlugin);
	const FString KnowledgeRoot = ResolveKnowledgeRoot(KbPlugin);

	for (const FString& RelOrAbs : RelativeOrAbsPaths)
	{
		FString Abs;
		if (FPaths::IsRelative(RelOrAbs))
		{
			FString Rel = RelOrAbs;
			Rel.ReplaceInline(TEXT("\\"), TEXT("/"));
			while (Rel.StartsWith(TEXT("./")))
			{
				Rel.RightChopInline(2);
			}
			if (Rel.StartsWith(TEXT("_raw/")))
			{
				Rel.RightChopInline(5);
			}
			if (Rel.Contains(TEXT("..")))
			{
				OutError = FString::Printf(TEXT("Rejected path with '..': %s"), *RelOrAbs);
				return false;
			}
			Abs = FPaths::ConvertRelativePathToFull(FPaths::Combine(RawRoot, Rel));
		}
		else
		{
			Abs = FPaths::ConvertRelativePathToFull(RelOrAbs);
		}

		if (!AxonLlmScopesPrivate::IsPathUnderDir(Abs, RawRoot)
			&& !AxonLlmScopesPrivate::IsPathUnderDir(Abs, KnowledgeRoot))
		{
			OutError = FString::Printf(TEXT("Path must be under KB Knowledge/ or _raw/: %s"), *Abs);
			return false;
		}
		if (!FPaths::FileExists(Abs))
		{
			OutError = FString::Printf(TEXT("Evidence file not found: %s"), *Abs);
			return false;
		}

		FString FileText;
		if (!FFileHelper::LoadFileToString(FileText, *Abs))
		{
			OutError = FString::Printf(TEXT("Failed to read: %s"), *Abs);
			return false;
		}

		const FString Header = FString::Printf(TEXT("\n\n===== EVIDENCE: %s =====\n"), *Abs);
		if (OutCombined.Len() + Header.Len() + FileText.Len() > MaxInputChars)
		{
			const int32 Remain = MaxInputChars - OutCombined.Len() - Header.Len();
			if (Remain <= 0)
			{
				bOutTruncated = true;
				break;
			}
			OutCombined += Header;
			OutCombined += FileText.Left(Remain);
			OutCombined += TEXT("\n...[truncated]...\n");
			OutUsedPaths.Add(Abs);
			bOutTruncated = true;
			break;
		}
		OutCombined += Header;
		OutCombined += FileText;
		OutUsedPaths.Add(Abs);
	}

	if (OutUsedPaths.Num() == 0)
	{
		OutError = TEXT("No evidence content loaded");
		return false;
	}
	return true;
}

bool FAxonLlmScopes::BuildRequestFromParams(
	EAxonLlmScope Scope,
	const FAxonLlmWorkerProfile& Worker,
	int32 WorkerIndex,
	const TSharedPtr<FJsonObject>& Params,
	FAxonLlmScopeRequest& OutReq,
	FString& OutError)
{
	const UAxonLLMSettings* Settings = UAxonLLMSettings::Get();
	OutReq = FAxonLlmScopeRequest();
	OutReq.Scope = Scope;
	OutReq.Worker = Worker;
	OutReq.WorkerIndex = WorkerIndex;
	OutReq.bDraftOnly = Settings ? Settings->bDraftOnly : true;
	OutReq.bDeleteDraftOnPromote = Settings ? Settings->bDeleteDraftOnPromote : true;
	OutReq.Paths = ParsePathsParam(Params);

	if (Params.IsValid())
	{
		Params->TryGetStringField(TEXT("kb_plugin"), OutReq.KbPlugin);
		Params->TryGetStringField(TEXT("topic"), OutReq.Topic);
		Params->TryGetStringField(TEXT("extra_instructions"), OutReq.ExtraInstructions);
		Params->TryGetStringField(TEXT("text"), OutReq.Text);
		Params->TryGetBoolField(TEXT("overwrite"), OutReq.bOverwrite);
	}
	if (OutReq.KbPlugin.IsEmpty() && Settings)
	{
		OutReq.KbPlugin = Settings->DefaultKbPlugin;
	}

	const bool bNeedsKb =
		Scope == EAxonLlmScope::KnowledgeSummarizeRaw
		|| Scope == EAxonLlmScope::KnowledgeDraftTopic
		|| Scope == EAxonLlmScope::KnowledgePromoteDraft;
	if (bNeedsKb && OutReq.KbPlugin.IsEmpty())
	{
		OutError = TEXT("kb_plugin is required (or set DefaultKbPlugin)");
		return false;
	}
	if (Scope == EAxonLlmScope::KnowledgeDraftTopic || Scope == EAxonLlmScope::KnowledgePromoteDraft)
	{
		if (OutReq.Topic.IsEmpty())
		{
			OutError = TEXT("topic is required");
			return false;
		}
		if (OutReq.Topic.Contains(TEXT("..")) || OutReq.Topic.Contains(TEXT("/")) || OutReq.Topic.Contains(TEXT("\\")))
		{
			OutError = TEXT("topic must be a plain filename stem");
			return false;
		}
	}
	if (Scope == EAxonLlmScope::LogSummarize && OutReq.Text.IsEmpty() && OutReq.Paths.Num() == 0)
	{
		OutError = TEXT("log.summarize requires text and/or paths");
		return false;
	}
	if ((Scope == EAxonLlmScope::KnowledgeSummarizeRaw || Scope == EAxonLlmScope::KnowledgeDraftTopic)
		&& OutReq.Paths.Num() == 0)
	{
		OutError = TEXT("paths is required");
		return false;
	}
	return true;
}

static FAxonActionResult ExecutePromote(const FAxonLlmScopeRequest& Req)
{
	const double Start = FPlatformTime::Seconds();
	FString FileName = Req.Topic;
	if (!FileName.EndsWith(TEXT(".md")))
	{
		FileName += TEXT(".md");
	}
	const FString FromPath = FPaths::ConvertRelativePathToFull(
		FPaths::Combine(FAxonLlmScopes::ResolveDraftRoot(Req.KbPlugin), FileName));
	const FString ToPath = FPaths::ConvertRelativePathToFull(
		FPaths::Combine(FAxonLlmScopes::ResolveKnowledgeRoot(Req.KbPlugin), FileName));

	if (!FPaths::FileExists(FromPath))
	{
		AxonLlmScopesPrivate::RecordUsage(Req, 0, 0, 0, false);
		return FAxonActionResult::Error(FString::Printf(TEXT("Draft not found: %s"), *FromPath));
	}
	if (FPaths::FileExists(ToPath) && !Req.bOverwrite)
	{
		AxonLlmScopesPrivate::RecordUsage(Req, 0, 0, 0, false);
		return FAxonActionResult::Error(
			FString::Printf(TEXT("Target exists (pass overwrite=true): %s"), *ToPath));
	}

	FString Content;
	if (!FFileHelper::LoadFileToString(Content, *FromPath))
	{
		return FAxonActionResult::Error(TEXT("Failed to read draft"));
	}
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(ToPath), true);
	if (!FFileHelper::SaveStringToFile(Content, *ToPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{
		return FAxonActionResult::Error(TEXT("Failed to write formal Knowledge md"));
	}
	if (Req.bDeleteDraftOnPromote)
	{
		IFileManager::Get().Delete(*FromPath);
	}

	const double Latency = (FPlatformTime::Seconds() - Start) * 1000.0;
	TSharedPtr<FJsonObject> Out = AxonLlmScopesPrivate::MakeRunBase(Req, Content.Len(), Content.Len(), Latency, false, {FromPath});
	Out->SetStringField(TEXT("kb_plugin"), Req.KbPlugin);
	Out->SetStringField(TEXT("topic"), Req.Topic);
	Out->SetStringField(TEXT("from_path"), FromPath);
	Out->SetStringField(TEXT("written_path"), ToPath);
	AxonLlmScopesPrivate::RecordUsage(Req, Content.Len(), Content.Len(), Latency, true);
	return FAxonActionResult::Success(Out);
}

static bool PrepLlmPrompt(
	const FAxonLlmScopeRequest& Req,
	FString& OutSystem,
	FString& OutUser,
	TArray<FString>& OutUsed,
	bool& bTruncated,
	FString& OutError)
{
	OutUsed.Reset();
	bTruncated = false;

	if (Req.Scope == EAxonLlmScope::KnowledgeSummarizeRaw || Req.Scope == EAxonLlmScope::KnowledgeDraftTopic)
	{
		FString Combined;
		if (!FAxonLlmScopes::ReadEvidenceFiles(
			Req.KbPlugin, Req.Paths, Req.Worker.MaxInputChars, Combined, OutUsed, bTruncated, OutError))
		{
			return false;
		}
		OutSystem = (Req.Scope == EAxonLlmScope::KnowledgeSummarizeRaw)
			? AxonLlmScopesPrivate::SummarizeSystem
			: AxonLlmScopesPrivate::DraftSystem;
		OutUser = TEXT("KB plugin: ");
		OutUser += Req.KbPlugin;
		if (Req.Scope == EAxonLlmScope::KnowledgeDraftTopic)
		{
			OutUser += TEXT("\nTopic file stem: ");
			OutUser += Req.Topic;
			OutUser += TEXT("\nWrite a full Knowledge topic draft from the evidence below.\n");
		}
		else
		{
			OutUser += TEXT("\nTask: summarize the following _raw evidence for an orchestrating Agent.\n");
		}
		if (!Req.ExtraInstructions.IsEmpty())
		{
			OutUser += TEXT("\nExtra instructions:\n");
			OutUser += Req.ExtraInstructions;
			OutUser += TEXT("\n");
		}
		OutUser += Combined;
		return true;
	}

	if (Req.Scope == EAxonLlmScope::LogSummarize)
	{
		OutSystem = AxonLlmScopesPrivate::LogSystem;
		FString Combined = Req.Text;
		if (Req.Paths.Num() > 0)
		{
			const FString SavedLogs = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Logs")));
			const FString ProjectSaved = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir());
			for (const FString& RelOrAbs : Req.Paths)
			{
				FString Abs = FPaths::IsRelative(RelOrAbs)
					? FPaths::ConvertRelativePathToFull(FPaths::Combine(SavedLogs, RelOrAbs))
					: FPaths::ConvertRelativePathToFull(RelOrAbs);
				if (RelOrAbs.Contains(TEXT("..")))
				{
					OutError = TEXT("Rejected path with '..'");
					return false;
				}
				if (!AxonLlmScopesPrivate::IsPathUnderDir(Abs, SavedLogs)
					&& !AxonLlmScopesPrivate::IsPathUnderDir(Abs, ProjectSaved)
					&& FPaths::IsRelative(RelOrAbs))
				{
					// allow absolute under Saved only
				}
				if (!AxonLlmScopesPrivate::IsPathUnderDir(Abs, ProjectSaved))
				{
					OutError = FString::Printf(TEXT("Log path must be under Project Saved/: %s"), *Abs);
					return false;
				}
				FString FileText;
				if (!FFileHelper::LoadFileToString(FileText, *Abs))
				{
					OutError = FString::Printf(TEXT("Failed to read log: %s"), *Abs);
					return false;
				}
				if (Combined.Len() + FileText.Len() > Req.Worker.MaxInputChars)
				{
					Combined += FileText.Left(Req.Worker.MaxInputChars - Combined.Len());
					bTruncated = true;
					OutUsed.Add(Abs);
					break;
				}
				Combined += TEXT("\n\n===== LOG: ");
				Combined += Abs;
				Combined += TEXT(" =====\n");
				Combined += FileText;
				OutUsed.Add(Abs);
			}
		}
		if (Combined.IsEmpty())
		{
			OutError = TEXT("No log text to summarize");
			return false;
		}
		if (Combined.Len() > Req.Worker.MaxInputChars)
		{
			Combined = Combined.Left(Req.Worker.MaxInputChars);
			bTruncated = true;
		}
		OutUser = TEXT("Summarize the following logs/errors.\n");
		if (!Req.ExtraInstructions.IsEmpty())
		{
			OutUser += TEXT("\nExtra instructions:\n");
			OutUser += Req.ExtraInstructions;
			OutUser += TEXT("\n");
		}
		OutUser += Combined;
		return true;
	}

	OutError = TEXT("Unsupported LLM scope");
	return false;
}

static FAxonActionResult FinalizeLlmResult(
	const FAxonLlmScopeRequest& Req,
	const FAxonLlmChatResult& Chat,
	bool bTruncated,
	const TArray<FString>& UsedPaths)
{
	if (!Chat.bOk)
	{
		AxonLlmScopesPrivate::RecordUsage(Req, Chat.CharsIn, 0, Chat.LatencyMs, false);
		return FAxonActionResult::Error(Chat.Error);
	}

	if (Req.Scope == EAxonLlmScope::KnowledgeDraftTopic)
	{
		const FString DraftRoot = FAxonLlmScopes::ResolveDraftRoot(Req.KbPlugin);
		IFileManager::Get().MakeDirectory(*DraftRoot, true);
		FString FileName = Req.Topic;
		if (!FileName.EndsWith(TEXT(".md")))
		{
			FileName += TEXT(".md");
		}
		const FString AbsPath = FPaths::ConvertRelativePathToFull(FPaths::Combine(DraftRoot, FileName));
		if (Req.bDraftOnly && !AxonLlmScopesPrivate::IsPathUnderDir(AbsPath, DraftRoot))
		{
			return FAxonActionResult::Error(TEXT("bDraftOnly=true: refusing write outside _draft/"));
		}
		if (!FFileHelper::SaveStringToFile(Chat.Content, *AbsPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
		{
			return FAxonActionResult::Error(FString::Printf(TEXT("Failed to write draft: %s"), *AbsPath));
		}
		TSharedPtr<FJsonObject> Out = AxonLlmScopesPrivate::MakeRunBase(
			Req, Chat.CharsIn, Chat.CharsOut, Chat.LatencyMs, bTruncated, UsedPaths);
		Out->SetStringField(TEXT("kb_plugin"), Req.KbPlugin);
		Out->SetStringField(TEXT("topic"), Req.Topic);
		Out->SetStringField(TEXT("written_path"), AbsPath);
		Out->SetStringField(TEXT("output_preview"), Chat.Content.Left(500));
		AxonLlmScopesPrivate::RecordUsage(Req, Chat.CharsIn, Chat.CharsOut, Chat.LatencyMs, true);
		return FAxonActionResult::Success(Out);
	}

	TSharedPtr<FJsonObject> Out = AxonLlmScopesPrivate::MakeRunBase(
		Req, Chat.CharsIn, Chat.CharsOut, Chat.LatencyMs, bTruncated, UsedPaths);
	if (!Req.KbPlugin.IsEmpty())
	{
		Out->SetStringField(TEXT("kb_plugin"), Req.KbPlugin);
	}
	Out->SetStringField(TEXT("output"), Chat.Content);
	AxonLlmScopesPrivate::RecordUsage(Req, Chat.CharsIn, Chat.CharsOut, Chat.LatencyMs, true);
	return FAxonActionResult::Success(Out);
}

FAxonActionResult FAxonLlmScopes::ExecuteScope(const FAxonLlmScopeRequest& Req)
{
	const FAxonLlmSyncBusyGuard BusyGuard(Req);

	if (Req.Scope == EAxonLlmScope::KnowledgePromoteDraft)
	{
		return ExecutePromote(Req);
	}

	FString System, User, Err;
	TArray<FString> Used;
	bool bTruncated = false;
	if (!PrepLlmPrompt(Req, System, User, Used, bTruncated, Err))
	{
		AxonLlmScopesPrivate::RecordUsage(Req, 0, 0, 0, false);
		return FAxonActionResult::Error(Err, -32602);
	}

	const FAxonLlmChatResult Chat = FAxonLlmClient::Chat(Req.Worker, System, User);
	return FinalizeLlmResult(Req, Chat, bTruncated, Used);
}

bool FAxonLlmScopes::ExecuteScopeAsync(
	const FAxonLlmScopeRequest& Req,
	TFunction<void(FAxonActionResult)> OnComplete)
{
	if (!ScopeNeedsLlm(Req.Scope))
	{
		OnComplete(ExecuteScope(Req));
		return true;
	}

	FString System, User, Err;
	TArray<FString> Used;
	bool bTruncated = false;
	if (!PrepLlmPrompt(Req, System, User, Used, bTruncated, Err))
	{
		AxonLlmScopesPrivate::RecordUsage(Req, 0, 0, 0, false);
		OnComplete(FAxonActionResult::Error(Err, -32602));
		return true;
	}

	FAxonLlmScopeRequest CapturedReq = Req;
	TArray<FString> CapturedUsed = Used;
	const bool bCapturedTrunc = bTruncated;

	FAxonLlmClient::ChatAsync(
		Req.Worker, System, User,
		FAxonLlmChatComplete::CreateLambda(
			[CapturedReq, CapturedUsed, bCapturedTrunc, OnComplete](FAxonLlmChatResult Chat)
			{
				OnComplete(FinalizeLlmResult(CapturedReq, Chat, bCapturedTrunc, CapturedUsed));
			}));
	return true;
}
