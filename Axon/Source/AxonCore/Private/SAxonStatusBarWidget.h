#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "AxonHttpServer.h"

/**
 * Level-editor status bar chip: colored light + "Axon MCP" label.
 * Red = off, Yellow = listening, Green = client connected recently.
 */
class SAxonStatusBarWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAxonStatusBarWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	EAxonMcpStatus GetStatus() const;
	FSlateColor GetLightColor() const;
	FText GetLabelText() const;
	FText GetToolTipText() const;
};
