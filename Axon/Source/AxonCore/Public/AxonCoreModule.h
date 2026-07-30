#pragma once

#include "Modules/ModuleManager.h"

#define AXON_VERSION TEXT("0.1.0")

class FAxonHttpServer;

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

private:
	TUniquePtr<FAxonHttpServer> HttpServer;
	FDelegateHandle ToolMenusStartupHandle;

	void RegisterCoreTools();
	void RegisterStatusBar();
	void UnregisterStatusBar();
	void WriteSentinelFile(int32 Port);
	void RemoveSentinelFile();
	FString GetSentinelFilePath() const;

	/** Touch plugin files if Axon.uplugin shows a future mtime (cross-TZ ZIP extraction artifact). */
	void NormalizeFutureMtimesIfNeeded();
};
