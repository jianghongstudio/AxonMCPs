#pragma once

#include "CoreMinimal.h"
#include "AxonToolRegistry.h"

/**
 * Editorial cross-namespace workflow guide for AI agents — primary audience is
 * EXTERNAL public-Axon users with no project CLAUDE.md routing or private skills.
 * Hybrid: hand-authored markdown at Plugins/Axon/Docs/axon_guide.md
 * + live registry overlay (action counts, gate status, plugin version).
 * Registered under the "Axon" namespace as action "guide".
 */
class FAxonGuideTool
{
public:
	/** Register the guide action (called from FAxonCoreTools::RegisterAll). */
	static void RegisterAll();

	/** Axon.guide — return the editorial guide, optionally filtered to a named H2 section. */
	static FAxonActionResult HandleGuide(const TSharedPtr<FJsonObject>& Params);

private:
	/** Load axon_guide.md from the plugin Docs/ dir. Cached after first successful load. */
	static bool LoadGuideMarkdown(FString& OutMarkdown, FString& OutErrorMessage);

	/** Split markdown into named sections keyed by H2 header ("## <name>"). */
	static void SplitSections(const FString& Markdown, TMap<FString, FString>& OutSections, TArray<FString>& OutOrderedNames);

	/** Live overlay JSON: per-namespace action counts + gate status + plugin version. */
	static TSharedPtr<FJsonObject> BuildLiveOverlay();
};
