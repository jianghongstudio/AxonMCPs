#pragma once

#include "Modules/ModuleManager.h"
#include "Templates/Function.h"

#define AXON_VERSION TEXT("0.1.0")

class FAxonHttpServer;

/** Snapshot for the status-bar worker HUD (filled by AxonLLM via provider). */
struct AXONCORE_API FAxonWorkerHudStatus
{
	bool bBusy = false;
	FString Model;
	int32 WorkerIndex = INDEX_NONE;
	FString ScopeWire;
	int32 QueueDepth = 0;
};

using FAxonWorkerHudProvider = TFunction<FAxonWorkerHudStatus()>;

class AXONCORE_API FAxonCoreModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	static inline FAxonCoreModule& Get()
	{
		return FModuleManager::LoadModuleChecked<FAxonCoreModule>("AxonCore");
	}

	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("AxonCore");
	}

	/** Get the running HTTP server instance */
	FAxonHttpServer* GetHttpServer() const { return HttpServer.Get(); }

	/** Console-command target: stop and restart the HTTP server on its configured port. */
	static void RestartHttpServer();

	/** AxonLLM (or others) register a provider; pass empty to clear. */
	void SetWorkerHudProvider(FAxonWorkerHudProvider Provider);
	FAxonWorkerHudStatus QueryWorkerHudStatus() const;

private:
	TUniquePtr<FAxonHttpServer> HttpServer;
	FDelegateHandle ToolMenusStartupHandle;
	FDelegateHandle DeferredHttpStartHandle;
	FAxonWorkerHudProvider WorkerHudProvider;

	void RegisterCoreTools();
	void RegisterStatusBar();
	void UnregisterStatusBar();
	void WriteSentinelFile(int32 Port);
	void RemoveSentinelFile();
	FString GetSentinelFilePath() const;

	/**
	 * Bind the MCP HTTP listener after sibling PostEngineInit modules have registered
	 * their namespaces. Starting earlier races Cursor/clients that tools/list before
	 * AxonLLM (worker_query) and other siblings finish StartupModule.
	 */
	void StartHttpServerIfEnabled();
	void CancelDeferredHttpStart();

	/** Touch plugin files if Axon.uplugin shows a future mtime (cross-TZ ZIP extraction artifact). */
	void NormalizeFutureMtimesIfNeeded();
};
