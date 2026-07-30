#include "SAxonStatusBarWidget.h"
#include "AxonCoreModule.h"
#include "AxonHttpServer.h"
#include "AxonSettings.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"

#define LOCTEXT_NAMESPACE "SAxonStatusBarWidget"

void SAxonStatusBarWidget::Construct(const FArguments& InArgs)
{
	SetToolTipText(TAttribute<FText>(this, &SAxonStatusBarWidget::GetToolTipText));

	ChildSlot
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

	switch (Status)
	{
	case EAxonMcpStatus::Connected:
		return FText::Format(
			LOCTEXT("AxonTipConnected", "Axon MCP: client connected (port {0})"),
			FText::AsNumber(BoundPort));
	case EAxonMcpStatus::Listening:
		return FText::Format(
			LOCTEXT("AxonTipListening", "Axon MCP: server listening on port {0}, waiting for client"),
			FText::AsNumber(BoundPort));
	case EAxonMcpStatus::Off:
	default:
		if (Settings && !Settings->bMcpServerEnabled)
		{
			return LOCTEXT("AxonTipDisabled", "Axon MCP: disabled in Project Settings → Plugins → Axon");
		}
		return LOCTEXT("AxonTipOff", "Axon MCP: server not running");
	}
}

#undef LOCTEXT_NAMESPACE
