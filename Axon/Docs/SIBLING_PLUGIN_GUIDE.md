# Axon — Sibling Plugin Guide

Extend Axon with a separate Unreal Editor plugin that registers MCP Actions into `FAxonToolRegistry` without modifying AxonCore.

Checklist + AI routing: [`../.Knowledges/50-extension-cookbook.md`](../.Knowledges/50-extension-cookbook.md) · user overview: [`USER_GUIDE.md`](USER_GUIDE.md).

## Scaffold via MCP (preferred)

Use the gated workflow in `Docs/axon_guide.md` → **Create an Axon extension plugin**:

1. Ask the user whether they want Action recommendations.
2. If yes: `axon_research_extension_target` → propose actions → user confirms.
3. `axon_create_extension` (prefer `dry_run=true` first) with confirmed `actions`, or `skeleton_only=true`.

Optional external recipes: `Templates/ExtensionRecipes/*.json` (Core ships none). List with `axon_list_extension_recipes`.

## Layout

All Axon MCP plugins (core + siblings) live under `Plugins/AxonMCPs/`.
`axon_create_extension` scaffolds new siblings there by default.

```
<Project>/Plugins/AxonMCPs/
  Axon/                 ← core (do not fork for game bridges)
  AxonAnimation/        ← domain sibling example
  MyBridge/             ← your sibling
    MyBridge.uplugin
    Source/MyBridge/
```

## .uplugin

```json
{
  "FileVersion": 3,
  "Version": 1,
  "VersionName": "0.1.0",
  "FriendlyName": "MyBridge",
  "Category": "Editor",
  "EnabledByDefault": true,
  "Plugins": [
    { "Name": "Axon", "Enabled": true }
  ],
  "Modules": [
    {
      "Name": "MyBridge",
      "Type": "Editor",
      "LoadingPhase": "Default"
    }
  ]
}
```

> **Note on LoadingPhase**: AxonCore now delays HTTP server start until `OnPostEngineInit`, which fires after all `Default` phase modules complete their `StartupModule()`. This ensures sibling plugins have registered their MCP actions before the server starts accepting connections.

## Build.cs

```csharp
PublicDependencyModuleNames.AddRange(new string[]
{
    "Core", "CoreUObject", "Engine", "UnrealEd", "AxonCore", "Json"
});
```

## Register actions

```cpp
#include "AxonToolRegistry.h"
#include "AxonParamSchema.h"

void FMyBridgeModule::StartupModule()
{
    FAxonToolRegistry& Registry = FAxonToolRegistry::Get();
    Registry.RegisterAction(
        TEXT("myns"),
        TEXT("do_x"),
        TEXT("What this action does."),
        FAxonActionHandler::CreateStatic(&HandleDoX),
        FParamSchemaBuilder()
            .Required(TEXT("asset_path"), TEXT("string"), TEXT("Target asset path"))
            .Build());
}

void FMyBridgeModule::ShutdownModule()
{
    if (FAxonCoreModule::IsAvailable())
    {
        FAxonToolRegistry::Get().UnregisterNamespace(TEXT("myns"));
    }
}
```

After rebuild + editor relaunch:

- `axon_discover()` lists `myns`
- MCP tool `myns_query` appears with `action` enum including `do_x`

## Optional bulk_fill / describe adapters

```cpp
FAxonBulkFillRegistry::Get().RegisterAdapter(
    TEXT("myns"),
    /*BulkFill*/ [](const FAxonBulkFillSpec& Spec) { return FAxonDryRunReport(); },
    /*Describe*/ [](const FString& Target) { return FAxonSchemaDescriptor(); });
```

Unregister with `UnregisterAdapter` in `ShutdownModule`.

## Rules

- Unique lowercase namespace; `snake_case` actions.
- Sibling module `LoadingPhase` should be `Default` (AxonCore delays HTTP start until `OnPostEngineInit`, ensuring all Default-phase modules complete first).
- Return `FAxonActionResult::Success(JsonObject)` or `Error(Message, Code)`.
- Do not register into the reserved `axon` / `describe` / `bulk_fill` namespaces.
- Scaffold stubs return `-32020` (`ErrNotImplemented`) until you replace the handlers.
- Never silently invent business Actions in Core; recommendation requires user consent + research.
