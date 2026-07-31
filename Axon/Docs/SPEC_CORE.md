# Axon Core — Specification

**Version:** 0.1.0  
**Default port:** 9320  
**Transport:** HTTP JSON-RPC 2.0 at `/mcp`  
**Product focus:** game **3C** (Character / Camera / Control) editor MCP

Human docs: [`USER_GUIDE.md`](USER_GUIDE.md) · AI knowledge: [`../.Knowledges/`](../.Knowledges/)

## Modules

| Module | Role |
|---|---|
| AxonCore | HTTP MCP server, Action registry, meta tools, describe/bulk_fill framework |
| AxonEditor (same uplugin) | `editor.*` PIE / build / capture actions |

Domain extensions are sibling plugins under `Plugins/AxonMCPs/` (scaffolded by `axon_create_extension`). Keep domain work (animation, GAS, etc.) out of Core.

## MCP tools

| Tool | Source |
|---|---|
| `axon_discover` | namespace `axon` / action `discover` |
| `axon_status` | namespace `axon` / action `status` |
| `axon_guide` | namespace `axon` / action `guide` |
| `axon_research_extension_target` | scan a target plugin (no Action recommendations) |
| `axon_create_extension` | scaffold sibling under `Plugins/AxonMCPs/` (requires actions or `skeleton_only`) |
| `axon_list_extension_recipes` | list optional external recipe JSON files |
| `describe_query` | namespace `describe` |
| `bulk_fill_query` | namespace `bulk_fill` |
| `{ns}_query` | any other registered namespace |

## Public extension API

- `FAxonToolRegistry` — `RegisterAction` / `UnregisterNamespace` / `ExecuteAction`
- `FAxonBulkFillRegistry` — `RegisterAdapter` / `UnregisterAdapter`
- `FParamSchemaBuilder` / `FAxonJsonUtils` / `FAxonAssetUtils`
- `UAxonSettings` — `bMcpServerEnabled`, `ServerPort`
- `FAxonCoreModule::IsAvailable()`

## Settings

Project Settings → Plugins → Axon:

- `bMcpServerEnabled` (default true)
- `ServerPort` (default 9320)
