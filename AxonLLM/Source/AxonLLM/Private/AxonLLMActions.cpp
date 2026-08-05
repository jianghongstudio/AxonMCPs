#include "AxonLLMActions.h"
#include "AxonLLMSettings.h"
#include "AxonLlmClient.h"
#include "AxonLlmRouter.h"
#include "AxonLlmScopes.h"
#include "AxonLlmJobQueue.h"
#include "AxonLlmUsage.h"
#include "AxonParamSchema.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Misc/Guid.h"

namespace AxonLLMActionsPrivate
{
	bool PrepareRoutedRequest(
		const TSharedPtr<FJsonObject>& Params,
		FAxonLlmScopeRequest& OutReq,
		FString& OutError)
	{
		const UAxonLLMSettings* Settings = UAxonLLMSettings::Get();
		if (!Settings || !Settings->bEnabled)
		{
			OutError = TEXT("AxonLLM is disabled (Project Settings → Plugins → Axon LLM)");
			return false;
		}
		if (!Params.IsValid())
		{
			OutError = TEXT("Missing params");
			return false;
		}

		FString ScopeStr;
		if (!Params->TryGetStringField(TEXT("scope"), ScopeStr) || ScopeStr.IsEmpty())
		{
			OutError = TEXT("scope is required");
			return false;
		}
		EAxonLlmScope ScopeEnum;
		if (!UAxonLLMSettings::WireToScope(ScopeStr, ScopeEnum))
		{
			OutError = FString::Printf(TEXT("Unknown scope '%s'"), *ScopeStr);
			return false;
		}

		const FAxonLlmRouteResult Route = FAxonLlmRouter::Resolve(ScopeEnum);
		if (!Route.bOk)
		{
			OutError = Route.Error;
			return false;
		}

		return FAxonLlmScopes::BuildRequestFromParams(
			ScopeEnum, Route.Worker, Route.WorkerIndex, Params, OutReq, OutError);
	}
}

void FAxonLLMActions::RegisterActions(FAxonToolRegistry& Registry)
{
	Registry.RegisterAction(
		TEXT("worker"), TEXT("status"),
		TEXT("Report AxonLLM enable flag, queue depth, and probe Ollama /api/tags."),
		FAxonActionHandler::CreateStatic(&FAxonLLMActions::HandleStatus),
		FParamSchemaBuilder().Build());

	Registry.RegisterAction(
		TEXT("worker"), TEXT("list"),
		TEXT("List configured workers (index, model, scopes). No network."),
		FAxonActionHandler::CreateStatic(&FAxonLLMActions::HandleList),
		FParamSchemaBuilder().Build());

	Registry.RegisterAction(
		TEXT("worker"), TEXT("run"),
		TEXT("Run a scoped local-LLM (or promote) job synchronously. Auto-picks first matching Workers[] entry."),
		FAxonActionHandler::CreateStatic(&FAxonLLMActions::HandleRun),
		FParamSchemaBuilder()
			.Required(TEXT("scope"), TEXT("string"),
				TEXT("knowledge.summarize_raw | knowledge.draft_topic | log.summarize | knowledge.promote_draft"))
			.Optional(TEXT("kb_plugin"), TEXT("string"), TEXT("KB plugin name; falls back to DefaultKbPlugin"))
			.Optional(TEXT("paths"), TEXT("array"), TEXT("Evidence or log paths"))
			.Optional(TEXT("topic"), TEXT("string"), TEXT("Topic stem for draft/promote"))
			.Optional(TEXT("text"), TEXT("string"), TEXT("Inline log text for log.summarize"))
			.Optional(TEXT("overwrite"), TEXT("boolean"), TEXT("promote_draft: overwrite existing Knowledge md"), TEXT("false"))
			.Optional(TEXT("extra_instructions"), TEXT("string"), TEXT("Optional prompt instructions"))
			.Build());

	Registry.RegisterAction(
		TEXT("worker"), TEXT("run_async"),
		TEXT("Enqueue a scoped job; returns job_id immediately. Poll with job_status."),
		FAxonActionHandler::CreateStatic(&FAxonLLMActions::HandleRunAsync),
		FParamSchemaBuilder()
			.Required(TEXT("scope"), TEXT("string"), TEXT("Same scopes as run"))
			.Optional(TEXT("kb_plugin"), TEXT("string"), TEXT("KB plugin name"))
			.Optional(TEXT("paths"), TEXT("array"), TEXT("Evidence or log paths"))
			.Optional(TEXT("topic"), TEXT("string"), TEXT("Topic stem"))
			.Optional(TEXT("text"), TEXT("string"), TEXT("Inline log text"))
			.Optional(TEXT("overwrite"), TEXT("boolean"), TEXT("promote overwrite"), TEXT("false"))
			.Optional(TEXT("extra_instructions"), TEXT("string"), TEXT("Optional instructions"))
			.Build());

	Registry.RegisterAction(
		TEXT("worker"), TEXT("job_status"),
		TEXT("Poll an async job by job_id."),
		FAxonActionHandler::CreateStatic(&FAxonLLMActions::HandleJobStatus),
		FParamSchemaBuilder()
			.Required(TEXT("job_id"), TEXT("string"), TEXT("Id returned by run_async"))
			.Build());

	Registry.RegisterAction(
		TEXT("worker"), TEXT("job_cancel"),
		TEXT("Cancel a queued async job (running jobs cannot be cancelled in P1)."),
		FAxonActionHandler::CreateStatic(&FAxonLLMActions::HandleJobCancel),
		FParamSchemaBuilder()
			.Required(TEXT("job_id"), TEXT("string"), TEXT("Id returned by run_async"))
			.Build());

	Registry.RegisterAction(
		TEXT("worker"), TEXT("usage_summary"),
		TEXT("Aggregate local worker usage from Saved JSONL logs."),
		FAxonActionHandler::CreateStatic(&FAxonLLMActions::HandleUsageSummary),
		FParamSchemaBuilder()
			.Optional(TEXT("days"), TEXT("integer"), TEXT("Lookback days (1-30)"), TEXT("7"))
			.Build());

	Registry.SetDispatcherAnnotations(TEXT("worker"),
		FAxonDispatcherAnnotations{false, false, false, TEXT("Axon local LLM workers")});
}

FAxonActionResult FAxonLLMActions::HandleList(const TSharedPtr<FJsonObject>& /*Params*/)
{
	const UAxonLLMSettings* Settings = UAxonLLMSettings::Get();
	TSharedPtr<FJsonObject> Out = MakeShared<FJsonObject>();
	Out->SetBoolField(TEXT("enabled"), Settings && Settings->bEnabled);
	Out->SetBoolField(TEXT("draft_only"), Settings ? Settings->bDraftOnly : true);
	Out->SetStringField(TEXT("default_kb_plugin"), Settings ? Settings->DefaultKbPlugin : FString());
	Out->SetNumberField(TEXT("queue_depth"), FAxonLlmJobQueue::Get().GetQueueDepth());
	Out->SetStringField(TEXT("running_job_id"), FAxonLlmJobQueue::Get().GetRunningJobId());

	TArray<TSharedPtr<FJsonValue>> WorkersJson;
	if (Settings)
	{
		for (int32 Index = 0; Index < Settings->Workers.Num(); ++Index)
		{
			const FAxonLlmWorkerProfile& W = Settings->Workers[Index];
			TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
			Obj->SetNumberField(TEXT("index"), Index);
			Obj->SetBoolField(TEXT("enabled"), W.bEnabled);
			Obj->SetStringField(TEXT("base_url"), W.BaseUrl);
			Obj->SetStringField(TEXT("model"), W.Model);
			Obj->SetNumberField(TEXT("timeout_sec"), W.TimeoutSec);
			TArray<TSharedPtr<FJsonValue>> ScopesJson;
			for (const FString& Wire : W.AllowedScopeWires())
			{
				ScopesJson.Add(MakeShared<FJsonValueString>(Wire));
			}
			Obj->SetArrayField(TEXT("scopes"), ScopesJson);
			WorkersJson.Add(MakeShared<FJsonValueObject>(Obj));
		}
	}
	Out->SetArrayField(TEXT("workers"), WorkersJson);

	TArray<TSharedPtr<FJsonValue>> Supported;
	for (const FString& Wire : UAxonLLMSettings::AllScopeWires())
	{
		Supported.Add(MakeShared<FJsonValueString>(Wire));
	}
	Out->SetArrayField(TEXT("supported_scopes"), Supported);
	return FAxonActionResult::Success(Out);
}

FAxonActionResult FAxonLLMActions::HandleStatus(const TSharedPtr<FJsonObject>& /*Params*/)
{
	const UAxonLLMSettings* Settings = UAxonLLMSettings::Get();
	TSharedPtr<FJsonObject> Out = MakeShared<FJsonObject>();
	const bool bEnabled = Settings && Settings->bEnabled;
	Out->SetBoolField(TEXT("enabled"), bEnabled);
	Out->SetNumberField(TEXT("queue_depth"), FAxonLlmJobQueue::Get().GetQueueDepth());
	Out->SetStringField(TEXT("running_job_id"), FAxonLlmJobQueue::Get().GetRunningJobId());

	if (!Settings)
	{
		return FAxonActionResult::Error(TEXT("AxonLLM settings unavailable"));
	}

	TArray<TSharedPtr<FJsonValue>> Probes;
	for (int32 Index = 0; Index < Settings->Workers.Num(); ++Index)
	{
		const FAxonLlmWorkerProfile& W = Settings->Workers[Index];
		TSharedPtr<FJsonObject> Probe = MakeShared<FJsonObject>();
		Probe->SetNumberField(TEXT("index"), Index);
		Probe->SetBoolField(TEXT("enabled"), W.bEnabled);
		Probe->SetStringField(TEXT("base_url"), W.BaseUrl);
		Probe->SetStringField(TEXT("model"), W.Model);

		if (!bEnabled || !W.bEnabled)
		{
			Probe->SetBoolField(TEXT("reachable"), false);
			Probe->SetStringField(TEXT("error"), TEXT("disabled"));
		}
		else
		{
			const FAxonLlmTagsResult Tags = FAxonLlmClient::GetTags(W.BaseUrl, FMath::Min(W.TimeoutSec, 8));
			Probe->SetBoolField(TEXT("reachable"), Tags.bOk);
			Probe->SetNumberField(TEXT("latency_ms"), Tags.LatencyMs);
			if (Tags.bOk)
			{
				TArray<TSharedPtr<FJsonValue>> Models;
				for (const FString& M : Tags.Models)
				{
					Models.Add(MakeShared<FJsonValueString>(M));
				}
				Probe->SetArrayField(TEXT("models"), Models);
				Probe->SetBoolField(TEXT("model_present"), Tags.Models.Contains(W.Model));
			}
			else
			{
				Probe->SetStringField(TEXT("error"), Tags.Error);
			}
		}
		Probes.Add(MakeShared<FJsonValueObject>(Probe));
	}
	Out->SetArrayField(TEXT("workers"), Probes);
	return FAxonActionResult::Success(Out);
}

FAxonActionResult FAxonLLMActions::HandleRun(const TSharedPtr<FJsonObject>& Params)
{
	FAxonLlmScopeRequest Req;
	FString Err;
	if (!AxonLLMActionsPrivate::PrepareRoutedRequest(Params, Req, Err))
	{
		return FAxonActionResult::Error(Err, -32602);
	}
	return FAxonLlmScopes::ExecuteScope(Req);
}

FAxonActionResult FAxonLLMActions::HandleRunAsync(const TSharedPtr<FJsonObject>& Params)
{
	FAxonLlmScopeRequest Req;
	FString Err;
	if (!AxonLLMActionsPrivate::PrepareRoutedRequest(Params, Req, Err))
	{
		return FAxonActionResult::Error(Err, -32602);
	}

	const FGuid Id = FAxonLlmJobQueue::Get().Enqueue(Req);
	if (!Id.IsValid())
	{
		return FAxonActionResult::Error(TEXT("Failed to enqueue job (module shutting down?)"));
	}

	TSharedPtr<FJsonObject> Out = MakeShared<FJsonObject>();
	Out->SetStringField(TEXT("job_id"), Id.ToString(EGuidFormats::DigitsWithHyphens));
	Out->SetStringField(TEXT("status"), TEXT("queued"));
	Out->SetNumberField(TEXT("worker_index"), Req.WorkerIndex);
	Out->SetStringField(TEXT("scope"), UAxonLLMSettings::ScopeToWire(Req.Scope));
	Out->SetNumberField(TEXT("queue_depth"), FAxonLlmJobQueue::Get().GetQueueDepth());
	return FAxonActionResult::Success(Out);
}

FAxonActionResult FAxonLLMActions::HandleJobStatus(const TSharedPtr<FJsonObject>& Params)
{
	if (!Params.IsValid())
	{
		return FAxonActionResult::Error(TEXT("Missing params"), -32602);
	}
	FString JobIdStr;
	if (!Params->TryGetStringField(TEXT("job_id"), JobIdStr) || JobIdStr.IsEmpty())
	{
		return FAxonActionResult::Error(TEXT("job_id is required"), -32602);
	}
	FGuid JobId;
	if (!FGuid::Parse(JobIdStr, JobId))
	{
		return FAxonActionResult::Error(TEXT("Invalid job_id"), -32602);
	}

	FAxonLlmJob Job;
	if (!FAxonLlmJobQueue::Get().GetStatus(JobId, Job))
	{
		return FAxonActionResult::Error(TEXT("Unknown job_id"), -32602);
	}

	TSharedPtr<FJsonObject> Out = MakeShared<FJsonObject>();
	Out->SetStringField(TEXT("job_id"), JobIdStr);
	Out->SetStringField(TEXT("status"), FAxonLlmJobQueue::StatusToString(Job.Status));
	Out->SetNumberField(TEXT("worker_index"), Job.Request.WorkerIndex);
	Out->SetStringField(TEXT("scope"), UAxonLLMSettings::ScopeToWire(Job.Request.Scope));

	if (Job.Status == EAxonLlmJobStatus::Succeeded && Job.Result.Result.IsValid())
	{
		Out->SetObjectField(TEXT("result"), Job.Result.Result);
	}
	if (Job.Status == EAxonLlmJobStatus::Failed || Job.Status == EAxonLlmJobStatus::Cancelled)
	{
		Out->SetStringField(TEXT("error"), Job.Error.IsEmpty() ? Job.Result.ErrorMessage : Job.Error);
	}
	return FAxonActionResult::Success(Out);
}

FAxonActionResult FAxonLLMActions::HandleJobCancel(const TSharedPtr<FJsonObject>& Params)
{
	if (!Params.IsValid())
	{
		return FAxonActionResult::Error(TEXT("Missing params"), -32602);
	}
	FString JobIdStr;
	Params->TryGetStringField(TEXT("job_id"), JobIdStr);
	FGuid JobId;
	if (!FGuid::Parse(JobIdStr, JobId))
	{
		return FAxonActionResult::Error(TEXT("Invalid job_id"), -32602);
	}
	FString Err;
	if (!FAxonLlmJobQueue::Get().Cancel(JobId, Err))
	{
		return FAxonActionResult::Error(Err, -32602);
	}
	TSharedPtr<FJsonObject> Out = MakeShared<FJsonObject>();
	Out->SetStringField(TEXT("job_id"), JobIdStr);
	Out->SetStringField(TEXT("status"), TEXT("cancelled"));
	return FAxonActionResult::Success(Out);
}

FAxonActionResult FAxonLLMActions::HandleUsageSummary(const TSharedPtr<FJsonObject>& Params)
{
	int32 Days = 7;
	if (Params.IsValid())
	{
		double D = 7;
		if (Params->TryGetNumberField(TEXT("days"), D))
		{
			Days = static_cast<int32>(D);
		}
	}
	return FAxonActionResult::Success(FAxonLlmUsage::BuildSummary(Days));
}
