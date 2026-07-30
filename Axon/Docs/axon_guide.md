# Axon Guide

## onboarding

Axon is an extensible Unreal Editor MCP core. It hosts an HTTP JSON-RPC server and a process-wide Action registry. Domain work lives in sibling plugins that call `FAxonToolRegistry::Get().RegisterAction(...)`.

1. Confirm the server: call `axon_status` (default port 9320).
2. List namespaces: `axon_discover()`.
3. List actions in a namespace: `axon_discover({ "namespace": "sample" })`.
4. Call a domain tool: `{namespace}_query` with `action` + params.
5. For one action's full param schema: `describe_query` with `action=action_schema`.

## recipes

### Create an Axon extension plugin (mandatory gate)

When the user wants a new domain bridge (any subsystem — do not assume a fixed domain):

1. **Ask first:** "Do you want me to recommend Actions for this extension?"
2. **If yes:** call `axon_research_extension_target` on the target plugin/path, read findings, **propose** an Action list, and **wait for user confirmation**.
3. **If no:** use the user's Action list, or `skeleton_only=true` for an empty shell.
4. Call `axon_create_extension` with confirmed `plugin_name` / `namespace` / `actions` (or skeleton_only). Prefer `dry_run=true` first. New plugins are written under `Plugins/AxonMCPs/<plugin_name>/`.
5. Close editor → UBT → relaunch → `axon_discover({namespace})`.

Never invent a business Action list without step 1–2 when the user asked for recommendations. Never skip research when recommendations were requested.

```
axon_research_extension_target({ "target_plugin": "SomePlugin" })
axon_create_extension({
  "plugin_name": "AxonSomePlugin",
  "namespace": "some_plugin",
  "depend_plugins": ["SomePlugin"],
  "build_modules": ["SomePlugin"],
  "actions": [ { "name": "inspect", "description": "..." } ],
  "dry_run": true
})
```

### Extend Axon from a sibling plugin (manual)

1. Create an Editor plugin under `Plugins/AxonMCPs/` that depends on `Axon` (uplugin) and `AxonCore` (Build.cs).
2. In `StartupModule`, register actions under a unique lowercase namespace.
3. In `ShutdownModule`, call `UnregisterNamespace`.
4. Rebuild, relaunch the editor, then `axon_discover()` to see the new namespace.

See also `Docs/SIBLING_PLUGIN_GUIDE.md`.

### Introspect an action schema

```
describe_query({ "action": "action_schema", "target_namespace": "sample", "target_action": "ping" })
```

### Bulk fill (requires a namespace adapter)

Sibling plugins that support reflective writes register via `FAxonBulkFillRegistry::RegisterAdapter`. Then:

```
bulk_fill_query({ "action": "apply", "target_namespace": "...", "target": "/Game/...", "tree": { ... }, "dry_run": true })
```

## decisions

| Need | Use |
|---|---|
| List what exists | `axon_discover` |
| Full params for one action | `describe_query` / `action_schema` |
| Server health / port | `axon_status` |
| Call domain work | `{ns}_query` |
| Research a plugin before recommending Actions | `axon_research_extension_target` |
| Scaffold a sibling extension | `axon_create_extension` |
| List optional external recipes | `axon_list_extension_recipes` |
| Reflective JSON tree write | `bulk_fill_query` (needs adapter) |

## errors

| Symptom | Fix |
|---|---|
| Connection refused on 9320 | Editor not running, plugin disabled, or port changed in Project Settings → Plugins → Axon |
| Unknown namespace | Call `axon_discover()`; sibling may not be loaded |
| Unknown action | Call `axon_discover({namespace})` |
| Optional dep unavailable (-32010) | Underlying marketplace/plugin missing |
| create_extension: actions empty | Ask user about recommendations; research if needed; pass actions or `skeleton_only=true` |
| Stub returns -32020 | Replace scaffolded handler with a real implementation |

## skills_map

| Area | Pointer |
|---|---|
| Sibling registration API | `Docs/SIBLING_PLUGIN_GUIDE.md` |
| Core architecture | `Docs/SPEC_CORE.md` |
| Public registry header | `Source/AxonCore/Public/AxonToolRegistry.h` |
| Extension templates | `Templates/ExtensionPlugin/` |
| Optional external recipes | `Templates/ExtensionRecipes/` |

## gotchas

- Core meta tools expand as `axon_*`; everything else is `{namespace}_query`.
- Default port is **9320** so Axon can coexist with Monolith on 9316.
- Sibling modules should use `LoadingPhase: PostEngineInit`.
- Claim a unique lowercase namespace; action names are `snake_case`.
- `bulk_fill` / `describe.schema` only work for namespaces that registered adapters; `describe.action_schema` works for any registered Action ParamSchema.
- Extension scaffolding is domain-agnostic: Core never ships built-in business recipes.
