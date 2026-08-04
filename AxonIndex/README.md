# AxonIndex

Project **UAsset** deep index for Axon MCP (`project_query`).

## Purpose

Full-project search over Blueprints (nodes/vars), materials, GAS-related assets, dependencies, and GameplayTags — backed by SQLite + FTS, not Content Browser UI.

## MCP

| Item | Value |
|------|--------|
| Namespace | `project` |
| Client tool | `project_query` |
| DB | `Saved/ProjectIndex.db` (under this plugin) |

Indexing runs **automatically** after the editor Asset Registry is ready (full then incremental). See `Axon/.Knowledges/32-index-and-source.md`.

## Common actions

- `search` / `find_by_type` / `find_references`
- `get_asset_details` / `get_stats`
- `list_gameplay_tags` / `search_gameplay_tags`
- `refresh_assets` / `cleanup_generated_assets` (sandbox `/Game/Tests/Axon/`)

## Related

- Sibling map: `Axon/.Knowledges/30-sibling-plugins.md`
- vs C++ index: **AxonSource** (`source_query`)
- vs offline docs: **AxonXxxKB** / `gasp_kb` (distill)
