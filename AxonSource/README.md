# AxonSource

**Engine + Project C++ / Shader** source index for Axon MCP (`source_query`).

## Purpose

Symbol search, signatures, callers/callees, class hierarchy, canonical includes, and usage examples — more than grepping files on disk.

## MCP

| Item | Value |
|------|--------|
| Namespace | `source` |
| Client tool | `source_query` |
| DB | `Saved/EngineSource.db` (under this plugin; path overridable) |

**Does not** auto-build a full index on startup. First use: `source_query` → `trigger_reindex`. Project-only refresh: `trigger_project_reindex`. See `Axon/.Knowledges/32-index-and-source.md`.

## Common actions

- `search_source` / `read_source` / `get_signature` / `get_symbol_context`
- `find_references` / `find_callers` / `find_callees` / `get_class_hierarchy`
- `get_include_path` / `find_example_usage`
- `trigger_reindex` / `trigger_project_reindex`

## Related

- Sibling map: `Axon/.Knowledges/30-sibling-plugins.md`
- vs asset index: **AxonIndex** (`project_query`)
- Gated by Axon settings `bEnableSource` when present
