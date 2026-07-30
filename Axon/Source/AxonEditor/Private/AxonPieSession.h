#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "UObject/WeakObjectPtr.h"

class UObject;
class FJsonObject;
class FJsonValue;

enum class EAxonPieSessionStatus : uint8
{
	Running,
	Complete,
	Stopped
};

enum class EAxonPieProvocationAction : uint8
{
	SetControlRotation,
	AddMovementInput,
	Jump,
	ConsoleCommand,
	Unknown
};

struct FAxonPieProvocation
{
	double AtSeconds = 0.0;
	EAxonPieProvocationAction Action = EAxonPieProvocationAction::Unknown;
	FString RawAction;
	FRotator Rotation = FRotator::ZeroRotator;
	FVector Direction = FVector::ForwardVector;
	double Scale = 1.0;
	FString Command;
	bool bFired = false;
	double FiredAtSeconds = -1.0;
	bool bDispatched = false;
	FString Result;
};

struct FAxonPieTimeseriesSample
{
	double TimeSeconds = 0.0;
	TArray<TPair<FString, TSharedPtr<FJsonValue>>> Vars;
};

struct FAxonPieSession
{
	FString Id;
	EAxonPieSessionStatus Status = EAxonPieSessionStatus::Running;
	double StartTimeSeconds = 0.0;
	double LastObservedSeconds = 0.0;
	double DurationSeconds = 6.0;
	double SampleInterval = 0.0;
	double LastSampleSeconds = -1.0;
	int32 MaxSamples = 2048;
	FString MapName;
	FString TargetActorLabel;
	FString TargetObjectName;
	FString TargetClassName;
	FString TargetComponentName;
	bool bTargetAnimInstance = false;
	bool bPieActive = true;
	bool bReady = false;
	bool bStoppedByTool = false;
	bool bTeardownStarted = false;
	TWeakObjectPtr<UObject> CachedTarget;
	TArray<FString> VariablePaths;
	TArray<FAxonPieProvocation> Provocations;
	TArray<FAxonPieTimeseriesSample> Samples;
};

class FAxonPieSessionManager
{
public:
	static FAxonPieSessionManager& Get();

	FString CreateSession(FAxonPieSession&& Session);
	FAxonPieSession* Find(const FString& SessionId);
	int32 Stop(const FString& SessionId);
	bool HasRunningSessions() const;
	TSharedPtr<FJsonObject> BuildReport(const FAxonPieSession& Session, bool bIncludeSamples) const;

private:
	bool OnFrameTick(float DeltaTime);
	void AdvanceSession(FAxonPieSession& Session);
	void EnsureObserver();
	void TeardownObserverIfIdle();
	void OnPieEnded(bool bIsSimulating);

	TMap<FString, FAxonPieSession> Sessions;
	FTSTicker::FDelegateHandle TickerHandle;
	FDelegateHandle EndPieHandle;
	FDelegateHandle PrePieEndedHandle;
	bool bObserverActive = false;
	uint32 NextSessionSerial = 1;
};
