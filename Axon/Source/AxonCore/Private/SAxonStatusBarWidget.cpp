#include "SAxonStatusBarWidget.h"
#include "AxonCoreModule.h"
#include "AxonHttpServer.h"
#include "AxonSettings.h"
#include "ISettingsModule.h"
#include "Modules/ModuleManager.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Styling/AppStyle.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Input/SComboButton.h"

#define LOCTEXT_NAMESPACE "SAxonStatusBarWidget"

void SAxonStatusBarWidget::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		SNew(SComboButton)
		.HasDownArrow(false)
		.ContentPadding(FMargin(0.f))
		.ForegroundColor(FSlateColor::UseForeground())
		.ButtonStyle(FAppStyle::Get(), "SimpleButton")
		.ToolTipText(this, &SAxonStatusBarWidget::GetToolTipText)
		.OnGetMenuContent(this, &SAxonStatusBarWidget::BuildMenu)
		.ButtonContent()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(6.f, 0.f, 3.f, 0.f)
			[
				SNew(SBox)
				.WidthOverride(10.f)
				.HeightOverride(10.f)
				[
					SNew(SBorder)
					.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
					.BorderBackgroundColor(this, &SAxonStatusBarWidget::GetLightColor)
					.Padding(0.f)
				]
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0.f, 0.f, 8.f, 0.f)
			[
				SNew(STextBlock)
				.Text(this, &SAxonStatusBarWidget::GetLabelText)
			]
		]
	];
}

EAxonMcpStatus SAxonStatusBarWidget::GetStatus() const
{
	if (!FAxonCoreModule::IsAvailable())
	{
		return EAxonMcpStatus::Off;
	}

	const FAxonHttpServer* Server = FAxonCoreModule::Get().GetHttpServer();
	if (!Server)
	{
		return EAxonMcpStatus::Off;
	}
	return Server->GetMcpStatus();
}

FAxonWorkerHudStatus SAxonStatusBarWidget::GetWorkerHud() const
{
	if (!FAxonCoreModule::IsAvailable())
	{
		return FAxonWorkerHudStatus();
	}
	return FAxonCoreModule::Get().QueryWorkerHudStatus();
}

FSlateColor SAxonStatusBarWidget::GetLightColor() const
{
	switch (GetStatus())
	{
	case EAxonMcpStatus::Connected:
		return FLinearColor(0.15f, 0.85f, 0.25f);
	case EAxonMcpStatus::Listening:
		return FLinearColor(0.95f, 0.75f, 0.1f);
	case EAxonMcpStatus::Off:
	default:
		return FLinearColor(0.9f, 0.15f, 0.15f);
	}
}

FText SAxonStatusBarWidget::GetLabelText() const
{
	const FAxonWorkerHudStatus Hud = GetWorkerHud();
	if (Hud.bBusy)
	{
		const FString BusyLabel = !Hud.Model.IsEmpty()
			? Hud.Model
			: (!Hud.ScopeWire.IsEmpty() ? Hud.ScopeWire : TEXT("busy"));
		return FText::Format(LOCTEXT("AxonBusyLabel", "Axon · {0}"), FText::FromString(BusyLabel));
	}
	return LOCTEXT("AxonMcpLabel", "Axon MCP");
}

FText SAxonStatusBarWidget::GetToolTipText() const
{
	const EAxonMcpStatus Status = GetStatus();
	const UAxonSettings* Settings = UAxonSettings::Get();
	const int32 Port = Settings ? Settings->ServerPort : 9320;

	FAxonHttpServer* Server = nullptr;
	if (FAxonCoreModule::IsAvailable())
	{
		Server = FAxonCoreModule::Get().GetHttpServer();
	}
	const int32 BoundPort = (Server && Server->IsRunning()) ? Server->GetPort() : Port;

	FString McpLine;
	switch (Status)
	{
	case EAxonMcpStatus::Connected:
		McpLine = FString::Printf(TEXT("MCP: client connected (port %d)"), BoundPort);
		break;
	case EAxonMcpStatus::Listening:
		McpLine = FString::Printf(TEXT("MCP: listening on port %d (waiting for client)"), BoundPort);
		break;
	case EAxonMcpStatus::Off:
	default:
		if (Settings && !Settings->bMcpServerEnabled)
		{
			McpLine = TEXT("MCP: disabled in Project Settings → Plugins → Axon");
		}
		else
		{
			McpLine = TEXT("MCP: server not running");
		}
		break;
	}

	const FAxonWorkerHudStatus Hud = GetWorkerHud();
	FString WorkerLine;
	if (Hud.bBusy)
	{
		WorkerLine = FString::Printf(
			TEXT("Worker: busy  model=%s  index=%d  scope=%s  queue=%d"),
			Hud.Model.IsEmpty() ? TEXT("(n/a)") : *Hud.Model,
			Hud.WorkerIndex,
			Hud.ScopeWire.IsEmpty() ? TEXT("?") : *Hud.ScopeWire,
			Hud.QueueDepth);
	}
	else
	{
		WorkerLine = FString::Printf(TEXT("Worker: idle  queue=%d"), Hud.QueueDepth);
	}

	return FText::FromString(McpLine + TEXT("\n") + WorkerLine + TEXT("\nClick: open Axon / Axon LLM settings"));
}

void SAxonStatusBarWidget::OpenProjectSettingsSection(const FName SectionName)
{
	if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
	{
		SettingsModule->ShowViewer(TEXT("Project"), TEXT("Plugins"), SectionName);
	}
}

TSharedRef<SWidget> SAxonStatusBarWidget::BuildMenu()
{
	FMenuBuilder MenuBuilder(true, nullptr);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("OpenAxonSettings", "打开 Axon 设置"),
		LOCTEXT("OpenAxonSettingsTip", "Project Settings → Plugins → Axon"),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateStatic(&SAxonStatusBarWidget::OpenProjectSettingsSection, FName(TEXT("AxonSettings")))));

	MenuBuilder.AddMenuEntry(
		LOCTEXT("OpenAxonLlmSettings", "打开 Axon LLM 设置"),
		LOCTEXT("OpenAxonLlmSettingsTip", "Project Settings → Plugins → Axon LLM"),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateStatic(&SAxonStatusBarWidget::OpenProjectSettingsSection, FName(TEXT("AxonLLMSettings")))));

	return MenuBuilder.MakeWidget();
}

#undef LOCTEXT_NAMESPACE
