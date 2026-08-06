#include "AxonCoreModule.h"
#include "AxonHttpServer.h"
#include "AxonSettings.h"
#include "AxonJsonUtils.h"
#include "AxonToolRegistry.h"
#include "AxonCoreTools.h"
#include "SAxonStatusBarWidget.h"
#include "Actions/AxonBulkFillActions.h"
#include "Misc/CoreDelegates.h"
#include "Misc/FileHelper.h"
#include "GenericPlatform/GenericPlatformProcess.h"
#include "Interfaces/IPluginManager.h"
#include "HAL/IConsoleManager.h"
#include "Containers/Ticker.h"
#include "ToolMenus.h"

#define LOCTEXT_NAMESPACE "FAxonCoreModule"

static FAutoConsoleCommand GAxonRestartCmd(
	TEXT("Axon.Restart"),
	TEXT("Restart the Axon MCP HTTP server on its configured port."),
	FConsoleCommandDelegate::CreateStatic(&FAxonCoreModule::RestartHttpServer)
);

void FAxonCoreModule::StartupModule()
{
	UE_LOG(LogAxon, Log, TEXT("Axon %s — Core module initializing"), AXON_VERSION);

	// Self-heal future-dated mtimes from cross-TZ ZIP extraction.
	NormalizeFutureMtimesIfNeeded();

	// Skip MCP server + sentinel in commandlets (cook/compile). The running editor already holds port 9320
	// and a second bind attempt surfaces as UAT ExitCode=1. Commandlets have no MCP consumer anyway.
	if (IsRunningCommandlet())
	{
		UE_LOG(LogAxon, Log, TEXT("Axon — commandlet detected, skipping MCP server startup"));
		return;
	}

	// Register core discovery/status tools
	RegisterCoreTools();

	// Phase 0: register bulk_fill + describe central dispatchers. Per-namespace
	// adapters self-register from their own module's StartupModule via
	// FAxonBulkFillRegistry::RegisterAdapter — those land in Phases 1-5.
	FAxonBulkFillActions::RegisterAll();

	// Defer HTTP bind until PostEngineInit to ensure ALL modules (including those
	// with deep dependency chains like AxonStructChooser) have completed their
	// StartupModule() and registered their MCP actions. OnAllModuleLoadingPhasesComplete
	// fires when phases transition, not when all StartupModule() calls finish.
	// PostEngineInit is the last initialization delegate, guaranteeing all modules ready.
	DeferredHttpStartHandle = FCoreDelegates::OnPostEngineInit.AddRaw(
		this, &FAxonCoreModule::StartHttpServerIfEnabled);
	// Fallback: if PostEngineInit already fired (hot-reload / late load),
	// start on the next ticker tick so we never leave MCP permanently off.
	FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateLambda([this](float)
		{
			StartHttpServerIfEnabled();
			return false;
		}),
		0.0f);

	// Status bar chip (red/yellow/green) — register once ToolMenus is ready.
	ToolMenusStartupHandle = UToolMenus::RegisterStartupCallback(
		FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FAxonCoreModule::RegisterStatusBar));
}

void FAxonCoreModule::SetWorkerHudProvider(FAxonWorkerHudProvider Provider)
{
	WorkerHudProvider = MoveTemp(Provider);
}

FAxonWorkerHudStatus FAxonCoreModule::QueryWorkerHudStatus() const
{
	if (WorkerHudProvider)
	{
		return WorkerHudProvider();
	}
	return FAxonWorkerHudStatus();
}

void FAxonCoreModule::ShutdownModule()
{
	WorkerHudProvider = nullptr;
	UnregisterStatusBar();
	CancelDeferredHttpStart();

	RemoveSentinelFile();

	if (HttpServer.IsValid())
	{
		HttpServer->Stop();
		HttpServer.Reset();
	}

	FAxonToolRegistry::Get().UnregisterNamespace(TEXT("axon"));
	FAxonBulkFillActions::UnregisterAll();

	UE_LOG(LogAxon, Log, TEXT("Axon — Core module shut down"));
}

void FAxonCoreModule::CancelDeferredHttpStart()
{
	if (DeferredHttpStartHandle.IsValid())
	{
		FCoreDelegates::OnAllModuleLoadingPhasesComplete.Remove(DeferredHttpStartHandle);
		DeferredHttpStartHandle.Reset();
	}
}

void FAxonCoreModule::StartHttpServerIfEnabled()
{
	CancelDeferredHttpStart();

	if (HttpServer.IsValid() && HttpServer->IsRunning())
	{
		return;
	}

	const UAxonSettings* Settings = UAxonSettings::Get();
	if (Settings && !Settings->bMcpServerEnabled)
	{
		UE_LOG(LogAxon, Log,
			TEXT("Axon — MCP server disabled in settings (bMcpServerEnabled=false), skipping HTTP listener startup"));
		return;
	}

	const int32 Port = Settings ? Settings->ServerPort : 9320;
	if (!HttpServer.IsValid())
	{
		HttpServer = MakeUnique<FAxonHttpServer>();
	}

	if (HttpServer->Start(Port))
	{
		WriteSentinelFile(Port);
		UE_LOG(LogAxon, Log,
			TEXT("Axon — MCP HTTP listening on %d after module load (%d namespaces, %d actions)"),
			Port,
			FAxonToolRegistry::Get().GetNamespaces().Num(),
			FAxonToolRegistry::Get().GetActionCount());
	}
	else
	{
		UE_LOG(LogAxon, Error, TEXT("Failed to start MCP server on port %d"), Port);
	}
}

void FAxonCoreModule::RegisterCoreTools()
{
	FAxonCoreTools::RegisterAll();
}

void FAxonCoreModule::RegisterStatusBar()
{
	UToolMenus::UnregisterOwner(this);
	FToolMenuOwnerScoped OwnerScoped(this);

	UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.StatusBar.ToolBar");
	FToolMenuSection& Section = Menu->FindOrAddSection("AxonMcp");
	Section.AddEntry(
		FToolMenuEntry::InitWidget(
			"AxonMcpStatus",
			SNew(SAxonStatusBarWidget),
			FText::GetEmpty(),
			true,
			false));

	UE_LOG(LogAxon, Log, TEXT("Axon MCP status bar widget registered"));
}

void FAxonCoreModule::UnregisterStatusBar()
{
	if (ToolMenusStartupHandle.IsValid())
	{
		UToolMenus::UnRegisterStartupCallback(ToolMenusStartupHandle);
		ToolMenusStartupHandle.Reset();
	}
	UToolMenus::UnregisterOwner(this);
}

FString FAxonCoreModule::GetSentinelFilePath() const
{
	TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("Axon"));
	if (Plugin.IsValid())
	{
		return Plugin->GetBaseDir() / TEXT("Saved") / TEXT(".axon_running");
	}
	return FPaths::Combine(FPaths::ProjectPluginsDir(), TEXT("AxonMCPs"), TEXT("Axon"), TEXT("Saved"), TEXT(".axon_running"));
}

void FAxonCoreModule::WriteSentinelFile(int32 Port)
{
	TSharedPtr<FJsonObject> Sentinel = MakeShared<FJsonObject>();
	Sentinel->SetNumberField(TEXT("pid"), FPlatformProcess::GetCurrentProcessId());
	Sentinel->SetNumberField(TEXT("port"), Port);
	Sentinel->SetStringField(TEXT("version"), AXON_VERSION);
	Sentinel->SetStringField(TEXT("started"), FDateTime::UtcNow().ToIso8601());

	FString Body;
	TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer =
		TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Body);
	FJsonSerializer::Serialize(Sentinel.ToSharedRef(), Writer);

	const FString Path = GetSentinelFilePath();
	if (FFileHelper::SaveStringToFile(Body, *Path, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{
		UE_LOG(LogAxon, Log, TEXT("Sentinel file written: %s"), *Path);
	}
	else
	{
		UE_LOG(LogAxon, Warning, TEXT("Failed to write sentinel file: %s"), *Path);
	}
}

void FAxonCoreModule::RemoveSentinelFile()
{
	const FString Path = GetSentinelFilePath();
	if (FPaths::FileExists(Path))
	{
		IFileManager::Get().Delete(*Path);
		UE_LOG(LogAxon, Log, TEXT("Sentinel file removed: %s"), *Path);
	}
}

void FAxonCoreModule::NormalizeFutureMtimesIfNeeded()
{
	TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("Axon"));
	if (!Plugin.IsValid())
	{
		return;
	}

	const FString PluginDir = Plugin->GetBaseDir();
	const FString UpluginPath = PluginDir / TEXT("Axon.uplugin");

	const FDateTime UpluginMtime = IFileManager::Get().GetTimeStamp(*UpluginPath);
	if (UpluginMtime == FDateTime::MinValue())
	{
		return;
	}

	const FDateTime NowUtc = FDateTime::UtcNow();
	if (UpluginMtime <= NowUtc)
	{
		return;
	}

	UE_LOG(LogAxon, Warning, TEXT("Axon.uplugin mtime %s is in the future (now %s) — normalizing plugin file timestamps"),
		*UpluginMtime.ToIso8601(), *NowUtc.ToIso8601());

	TArray<FString> AllFiles;
	IFileManager::Get().FindFilesRecursive(AllFiles, *PluginDir, TEXT("*"), true, false);

	int32 Touched = 0;
	int32 Failed = 0;
	for (const FString& File : AllFiles)
	{
		if (IFileManager::Get().SetTimeStamp(*File, NowUtc)) { ++Touched; } else { ++Failed; }
	}

	UE_LOG(LogAxon, Log, TEXT("Normalized %d file(s), %d failed"), Touched, Failed);
}

void FAxonCoreModule::RestartHttpServer()
{
	if (!IsAvailable())
	{
		UE_LOG(LogAxon, Warning, TEXT("Axon.Restart: AxonCore module not loaded"));
		return;
	}

	FAxonCoreModule& Module = Get();
	Module.CancelDeferredHttpStart();

	const UAxonSettings* Settings = UAxonSettings::Get();
	if (Settings && !Settings->bMcpServerEnabled)
	{
		UE_LOG(LogAxon, Warning, TEXT("Axon.Restart: bMcpServerEnabled=false — not starting"));
		return;
	}

	const int32 Port = Settings ? Settings->ServerPort : 9320;
	if (!Module.HttpServer.IsValid())
	{
		Module.HttpServer = MakeUnique<FAxonHttpServer>();
	}

	UE_LOG(LogAxon, Log, TEXT("Axon.Restart: restarting HTTP server on port %d"), Port);
	if (Module.HttpServer->Restart(Port))
	{
		Module.WriteSentinelFile(Port);
		UE_LOG(LogAxon, Log, TEXT("Axon.Restart: success (%d namespaces)"),
			FAxonToolRegistry::Get().GetNamespaces().Num());
	}
	else
	{
		UE_LOG(LogAxon, Error, TEXT("Axon.Restart: failed to rebind port %d"), Port);
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FAxonCoreModule, AxonCore)
