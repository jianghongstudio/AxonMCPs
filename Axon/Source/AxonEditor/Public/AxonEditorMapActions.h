// AxonEditorMapActions.h
// Phase F8 (J-phase) — empty map authoring + module/plugin reflection.
//
//   editor::create_empty_map       — Create a fully blank UWorld asset on disk
//                                    via UWorldFactory + IAssetTools::CreateAsset.
//   editor::get_module_status      — Reflect plugin enable + module load status
//                                    for Axon (or arbitrary) modules. Wraps
//                                    IPluginManager::GetDiscoveredPlugins +
//                                    FModuleManager::IsModuleLoaded.
//
// Both actions are project-agnostic (no Leviathan-specific symbols) and live in
// the editor-only AxonEditor module. Registration is invoked from
// FAxonEditorModule::StartupModule via FAxonEditorMapActions::RegisterActions.
#pragma once

#include "CoreMinimal.h"
#include "AxonToolRegistry.h"

class FAxonEditorMapActions
{
public:
	static void RegisterActions(FAxonToolRegistry& Registry);

	static FAxonActionResult HandleCreateEmptyMap(const TSharedPtr<FJsonObject>& Params);
	static FAxonActionResult HandleGetModuleStatus(const TSharedPtr<FJsonObject>& Params);
};
