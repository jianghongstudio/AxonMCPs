#pragma once

#include "CoreMinimal.h"
#include "AxonLlmScopes.h"
#include "AxonToolRegistry.h"
#include "AxonCoreModule.h"

enum class EAxonLlmJobStatus : uint8
{
	Queued,
	Running,
	Succeeded,
	Failed,
	Cancelled
};

struct FAxonLlmJob
{
	FGuid Id;
	EAxonLlmJobStatus Status = EAxonLlmJobStatus::Queued;
	FAxonLlmScopeRequest Request;
	FAxonActionResult Result;
	FString Error;
	double EnqueuedAt = 0.0;
	double FinishedAt = 0.0;
};

class FAxonLlmJobQueue
{
public:
	static FAxonLlmJobQueue& Get();

	FGuid Enqueue(const FAxonLlmScopeRequest& Request);
	bool Cancel(const FGuid& JobId, FString& OutError);
	bool GetStatus(const FGuid& JobId, FAxonLlmJob& OutJob) const;

	int32 GetQueueDepth() const;
	FString GetRunningJobId() const;

	/** Busy snapshot for status bar (async RunningId preferred over sync busy). */
	FAxonWorkerHudStatus GetBusySnapshot() const;

	void BeginSyncBusy(const FAxonLlmScopeRequest& Request);
	void EndSyncBusy();

	void Shutdown();

	static FString StatusToString(EAxonLlmJobStatus Status);

private:
	void Pump();
	void CompleteJob(const FGuid& JobId, const FAxonActionResult& Result);

	mutable FCriticalSection Lock;
	TMap<FGuid, FAxonLlmJob> Jobs;
	TArray<FGuid> QueueOrder;
	FGuid RunningId;
	bool bShuttingDown = false;

	bool bSyncBusy = false;
	FAxonLlmScopeRequest SyncBusyRequest;

	static constexpr int32 MaxRetainedJobs = 32;
};

/** RAII: mark sync ExecuteScope as busy for the status bar. */
struct FAxonLlmSyncBusyGuard
{
	explicit FAxonLlmSyncBusyGuard(const FAxonLlmScopeRequest& Request)
	{
		FAxonLlmJobQueue::Get().BeginSyncBusy(Request);
	}
	~FAxonLlmSyncBusyGuard()
	{
		FAxonLlmJobQueue::Get().EndSyncBusy();
	}
};
