#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "AxonHttpServer.h"
#include "AxonCoreModule.h"

/**
 * Level-editor status bar chip: colored light + label.
 * Red/Yellow/Green = MCP; label shows busy worker model when AxonLLM reports activity.
 * Click → menu to open Axon / Axon LLM Project Settings.
 */
class SAxonStatusBarWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAxonStatusBarWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	EAxonMcpStatus GetStatus() const;
	FAxonWorkerHudStatus GetWorkerHud() const;
	FSlateColor GetLightColor() const;
	FText GetLabelText() const;
	FText GetToolTipText() const;
	TSharedRef<SWidget> BuildMenu();
	static void OpenProjectSettingsSection(const FName SectionName);
};
