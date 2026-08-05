#include "AxonLlmJobQueue.h"
#include "AxonLLMSettings.h"

FAxonLlmJobQueue& FAxonLlmJobQueue::Get()
{
	static FAxonLlmJobQueue Instance;
	return Instance;
}

FString FAxonLlmJobQueue::StatusToString(EAxonLlmJobStatus Status)
{
	switch (Status)
	{
	case EAxonLlmJobStatus::Queued: return TEXT("queued");
	case EAxonLlmJobStatus::Running: return TEXT("running");
	case EAxonLlmJobStatus::Succeeded: return TEXT("succeeded");
	case EAxonLlmJobStatus::Failed: return TEXT("failed");
	case EAxonLlmJobStatus::Cancelled: return TEXT("cancelled");
	default: return TEXT("unknown");
	}
}

FGuid FAxonLlmJobQueue::Enqueue(const FAxonLlmScopeRequest& Request)
{
	FGuid Id = FGuid::NewGuid();
	{
		FScopeLock Guard(&Lock);
		if (bShuttingDown)
		{
			return FGuid();
		}
		FAxonLlmJob Job;
		Job.Id = Id;
		Job.Status = EAxonLlmJobStatus::Queued;
		Job.Request = Request;
		Job.Request.JobId = Id.ToString(EGuidFormats::DigitsWithHyphens);
		Job.EnqueuedAt = FPlatformTime::Seconds();
		Jobs.Add(Id, Job);
		QueueOrder.Add(Id);
	}
	Pump();
	return Id;
}

bool FAxonLlmJobQueue::Cancel(const FGuid& JobId, FString& OutError)
{
	FScopeLock Guard(&Lock);
	FAxonLlmJob* Job = Jobs.Find(JobId);
	if (!Job)
	{
		OutError = TEXT("Unknown job_id");
		return false;
	}
	if (Job->Status == EAxonLlmJobStatus::Queued)
	{
		Job->Status = EAxonLlmJobStatus::Cancelled;
		Job->FinishedAt = FPlatformTime::Seconds();
		Job->Error = TEXT("cancelled while queued");
		QueueOrder.Remove(JobId);
		return true;
	}
	if (Job->Status == EAxonLlmJobStatus::Running)
	{
		OutError = TEXT("Job already running; cannot cancel reliably in P1 (wait for completion)");
		return false;
	}
	OutError = TEXT("Job already finished");
	return false;
}

bool FAxonLlmJobQueue::GetStatus(const FGuid& JobId, FAxonLlmJob& OutJob) const
{
	FScopeLock Guard(&Lock);
	const FAxonLlmJob* Job = Jobs.Find(JobId);
	if (!Job)
	{
		return false;
	}
	OutJob = *Job;
	return true;
}

int32 FAxonLlmJobQueue::GetQueueDepth() const
{
	FScopeLock Guard(&Lock);
	return QueueOrder.Num() + (RunningId.IsValid() ? 1 : 0) + (bSyncBusy ? 1 : 0);
}

FString FAxonLlmJobQueue::GetRunningJobId() const
{
	FScopeLock Guard(&Lock);
	return RunningId.IsValid() ? RunningId.ToString(EGuidFormats::DigitsWithHyphens) : FString();
}

FAxonWorkerHudStatus FAxonLlmJobQueue::GetBusySnapshot() const
{
	FScopeLock Guard(&Lock);
	FAxonWorkerHudStatus Out;
	Out.QueueDepth = QueueOrder.Num() + (RunningId.IsValid() ? 1 : 0) + (bSyncBusy ? 1 : 0);

	if (RunningId.IsValid())
	{
		if (const FAxonLlmJob* Job = Jobs.Find(RunningId))
		{
			Out.bBusy = true;
			Out.Model = Job->Request.Worker.Model;
			Out.WorkerIndex = Job->Request.WorkerIndex;
			Out.ScopeWire = UAxonLLMSettings::ScopeToWire(Job->Request.Scope);
			return Out;
		}
	}

	if (bSyncBusy)
	{
		Out.bBusy = true;
		Out.Model = SyncBusyRequest.Worker.Model;
		Out.WorkerIndex = SyncBusyRequest.WorkerIndex;
		Out.ScopeWire = UAxonLLMSettings::ScopeToWire(SyncBusyRequest.Scope);
	}
	return Out;
}

void FAxonLlmJobQueue::BeginSyncBusy(const FAxonLlmScopeRequest& Request)
{
	FScopeLock Guard(&Lock);
	bSyncBusy = true;
	SyncBusyRequest = Request;
}

void FAxonLlmJobQueue::EndSyncBusy()
{
	FScopeLock Guard(&Lock);
	bSyncBusy = false;
	SyncBusyRequest = FAxonLlmScopeRequest();
}

void FAxonLlmJobQueue::Shutdown()
{
	FScopeLock Guard(&Lock);
	bShuttingDown = true;
	bSyncBusy = false;
	QueueOrder.Reset();
	for (auto& Pair : Jobs)
	{
		if (Pair.Value.Status == EAxonLlmJobStatus::Queued)
		{
			Pair.Value.Status = EAxonLlmJobStatus::Cancelled;
			Pair.Value.Error = TEXT("module shutdown");
		}
	}
}

void FAxonLlmJobQueue::CompleteJob(const FGuid& JobId, const FAxonActionResult& Result)
{
	{
		FScopeLock Guard(&Lock);
		FAxonLlmJob* Job = Jobs.Find(JobId);
		if (Job)
		{
			Job->Result = Result;
			Job->Status = Result.bSuccess ? EAxonLlmJobStatus::Succeeded : EAxonLlmJobStatus::Failed;
			if (!Result.bSuccess)
			{
				Job->Error = Result.ErrorMessage;
			}
			Job->FinishedAt = FPlatformTime::Seconds();
		}
		if (RunningId == JobId)
		{
			RunningId.Invalidate();
		}

		TArray<FGuid> Finished;
		for (const auto& Pair : Jobs)
		{
			if (Pair.Value.Status == EAxonLlmJobStatus::Succeeded
				|| Pair.Value.Status == EAxonLlmJobStatus::Failed
				|| Pair.Value.Status == EAxonLlmJobStatus::Cancelled)
			{
				Finished.Add(Pair.Key);
			}
		}
		Finished.Sort([this](const FGuid& A, const FGuid& B)
		{
			return Jobs[A].FinishedAt > Jobs[B].FinishedAt;
		});
		for (int32 i = MaxRetainedJobs; i < Finished.Num(); ++i)
		{
			Jobs.Remove(Finished[i]);
		}
	}
	Pump();
}

void FAxonLlmJobQueue::Pump()
{
	FAxonLlmScopeRequest Req;
	FGuid JobId;
	{
		FScopeLock Guard(&Lock);
		if (bShuttingDown || RunningId.IsValid() || QueueOrder.Num() == 0)
		{
			return;
		}
		JobId = QueueOrder[0];
		QueueOrder.RemoveAt(0);
		FAxonLlmJob* Job = Jobs.Find(JobId);
		if (!Job || Job->Status != EAxonLlmJobStatus::Queued)
		{
			return;
		}
		Job->Status = EAxonLlmJobStatus::Running;
		RunningId = JobId;
		Req = Job->Request;
	}

	FAxonLlmScopes::ExecuteScopeAsync(Req, [this, JobId](FAxonActionResult Result)
	{
		CompleteJob(JobId, Result);
	});
}
