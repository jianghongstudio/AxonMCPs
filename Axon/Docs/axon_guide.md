# Axon Guide

> Human onboarding (中文)：[`USER_GUIDE.md`](USER_GUIDE.md) · 3C workflows：[`3C_WORKFLOWS.md`](3C_WORKFLOWS.md)  
> AI knowledge base：[`../.Knowledges/00-routing.md`](../.Knowledges/00-routing.md) · suite index：[`../README.md`](../README.md)

## onboarding

Axon is an extensible Unreal Editor MCP core aimed at game **3C** (Character / Camera / Control) authoring and PIE verification. It hosts an HTTP JSON-RPC server and a process-wide Action registry. Domain work lives in sibling plugins that call `FAxonToolRegistry::Get().RegisterAction(...)`.

It also supports **project distillation**: turn the current project into a standalone offline KB sibling (`Axon{Project}KB`) via `knowledge_query` / `scaffold_kb_plugin`. If the user says 蒸馏 / distill, follow the **Distill current project** recipe below (mandatory). Details: `../.Knowledges/31-knowledge-distill.md`.

1. Confirm the server: call `axon_status` (default port 9320).
2. List namespaces: `axon_discover()`.
3. List actions in a namespace: `axon_discover({ "namespace": "sample" })`.
4. Call a domain tool: `{namespace}_query` with `action` + params.
5. For one action's full param schema: `describe_query` with `action=action_schema`.
6. For project assets / refs: `project_query` (AxonIndex). For engine C++: `source_query` (AxonSource; `trigger_reindex` if empty).
7. For offline project knowledge: enable `AxonKnowledgeLib`; after distill use `{project}_kb_query` (example pack: `gasp_kb`).

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

### Edit graph nodes with layout awareness

When adding or swapping nodes in AnimGraphs or Blueprint graphs:

1. **Replace in place:** `animation.replace_anim_graph_node` or `blueprint.replace_blueprint_node` — preserves coordinates and reconnects by pin name.
2. **Add adjacent:** `add_anim_graph_node` / `add_node` with `anchor_node` + `anchor_mode` (`at`|`right`|`below`) instead of raw coordinates.
3. **Tidy after local edits:** `auto_layout` with `layout_mode=selected` or `new_only`; query positions via `animation.get_anim_node_positions`.

### Introspect an action schema

```
describe_query({ "action": "action_schema", "target_namespace": "sample", "target_action": "ping" })
```

### Bulk fill (requires a namespace adapter)

Sibling plugins that support reflective writes register via `FAxonBulkFillRegistry::RegisterAdapter`. Then:

```
bulk_fill_query({ "action": "apply", "target_namespace": "...", "target": "/Game/...", "tree": { ... }, "dry_run": true })
```

### Search project assets or engine C++ (Index / Source)

| Need | Tool | Notes |
|---|---|---|
| Find assets, BP nodes, tags, dependency graph | `project_query` (AxonIndex) | Index builds automatically after editor Asset Registry is ready |
| C++ / shader API, callers, includes, examples | `source_query` (AxonSource) | First time: `trigger_reindex` (full engine+project). Then `get_signature` / `find_callers` / `search_source` |

Do not confuse with offline KB packs (`gasp_kb` / `{project}_kb`). Details: `../.Knowledges/32-index-and-source.md`.

```
project_query({ "action": "search", "query": "StateController" })
project_query({ "action": "find_references", "asset_path": "/Game/..." })
source_query({ "action": "trigger_reindex" })
source_query({ "action": "get_signature", "symbol": "..." })
```

### Distill current project into a standalone KB plugin (mandatory)

**Trigger phrases (treat as this recipe — do not skip scaffolding):**
- Chinese: `蒸馏本项目` / `蒸馏` + project name
- English: `distill this project` / `distill` + project name

When the user asks to distill **any** Axon-enabled project into offline knowledge:

1. **Derive names** from the current project (`axon_status` / `FApp` project name), or call:
   `knowledge_query({ "action": "preview_kb_names" })`
   → `plugin_name=Axon{Project}KB`, `namespace={snake_project}_kb`
   (Existing exception: Game Animation Sample stays `AxonGaspKB` / `gasp_kb` — do not re-scaffold it.)
2. **Scaffold** the independent sibling plugin (Knowledge templates + thin RegisterAll):
   `knowledge_query({ "action": "scaffold_kb_plugin", "dry_run": true })` then without `dry_run`.
   Do **not** use generic `axon_create_extension` for KB packs (it will not wire AxonKnowledgeLib / Knowledge/).
3. **Close editor → UBT → relaunch** (required for the new `.uplugin` / module).
4. **Extract** evidence into that plugin's `Knowledge/_raw/` via `{ns}_query`:
   `extract_state_machines` / `extract_anim_graph_overview` / `extract_chooser` / `extract_config_ddcvars` / `extract_bundle` / `extract_invoke`.
5. **Distill** `Knowledge/*.md` from `_raw` (Agent-authored markdown; C++ does not auto-generate docs). Depth bar: match AxonGaspKB (State Controller–level detail for major systems).
   - **Optional local workers (AxonLLM)**: `worker_query` → `status` / `run` / `run_async` with scopes `knowledge.summarize_raw`, `knowledge.draft_topic` (`_draft/{topic}.md`), then `knowledge.promote_draft` to formal topics. Long jobs: `run_async` + `job_status`. Do not treat drafts as published Knowledge.
6. **Accept** with `{ns}_query` `search` / `read` / `list_topics`.
7. Copying the whole `AxonXxxKB/` folder to another project is a human decision (enable/disable plugin).

```
knowledge_query({ "action": "preview_kb_names" })
knowledge_query({ "action": "scaffold_kb_plugin", "dry_run": true })
knowledge_query({ "action": "scaffold_kb_plugin" })
# close editor → UBT → relaunch
{ns}_query({ "action": "extract_bundle", "jobs": [ ... ] })
# optional: worker_query({ "action": "run", "scope": "knowledge.draft_topic", ... })
{ns}_query({ "action": "search", "query": "..." })
```

Requires sibling plugin **AxonKnowledgeLib** (`knowledge` namespace). Each distilled project is its own `AxonXxxKB` plugin — never dump multi-project corpora into one host. Optional **AxonLLM** (`worker` namespace) for local Ollama drafts — see `../AxonLLM/README.md`.

### Local LLM workers (optional)

When **AxonLLM** is enabled and Ollama is reachable:

| Need | Call |
|---|---|
| Probe workers / models | `worker_query` / `status` or `list` |
| Compress `_raw` for the Agent | `worker_query` / `run` + `scope=knowledge.summarize_raw` + `kb_plugin` + `paths` (auto-picks Workers[] by index order) |
| Draft a topic under `_draft/` | `worker_query` / `run` or `run_async` + `scope=knowledge.draft_topic` + `topic` + `paths` |
| Promote draft → formal md | `worker_query` / `run` + `scope=knowledge.promote_draft` + `topic` (+ optional `overwrite`) |
| Summarize build/logs | `worker_query` / `run` + `scope=log.summarize` + `text` and/or Saved `paths` |
| Usage totals | `worker_query` / `usage_summary` |

Workers never recurse into other MCP actions. Promote is explicit (`knowledge.promote_draft`).

## decisions

| Need | Use |
|---|---|
| List what exists | `axon_discover` |
| Full params for one action | `describe_query` / `action_schema` |
| Server health / port | `axon_status` |
| Call domain work | `{ns}_query` |
| Research a plugin before recommending Actions | `axon_research_extension_target` |
| Scaffold a sibling extension | `axon_create_extension` |
| Distill project → standalone KB plugin | `knowledge_query` / `scaffold_kb_plugin` (see Distill recipe) |
| Preview KB plugin/namespace names | `knowledge_query` / `preview_kb_names` |
| List registered KB packs | `knowledge_query` / `list_kb_packs` |
| Local LLM summarize / draft / promote | `worker_query` / `status` / `list` / `run` / `run_async` / `job_*` / `usage_summary` (AxonLLM) |
| Search project assets / refs / tags | `project_query` (AxonIndex) |
| Search engine/project C++ / callers | `source_query` (AxonSource; reindex first if empty) |
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
| Unknown namespace `knowledge` | Enable/load **AxonKnowledgeLib**; relaunch after UBT |
| Distill asked but no new plugin | Follow Distill recipe: `scaffold_kb_plugin` first, then compile — not `create_extension` |
| `source` empty / missing symbols | `source_query` → `trigger_reindex` (full). Wait for completion |
| `project` stale results | `refresh_assets` or wait for startup incremental index |

## skills_map

| Area | Pointer |
|---|---|
| Sibling registration API | `Docs/SIBLING_PLUGIN_GUIDE.md` |
| Core architecture | `Docs/SPEC_CORE.md` |
| Public registry header | `Source/AxonCore/Public/AxonToolRegistry.h` |
| Extension templates | `Templates/ExtensionPlugin/` |
| Optional external recipes | `Templates/ExtensionRecipes/` |
| KB pack lib + scaffold | `Plugins/AxonMCPs/AxonKnowledgeLib/` |
| GASP offline corpus example | `Plugins/AxonMCPs/AxonGaspKB/` |
| Project asset index | `Plugins/AxonMCPs/AxonIndex/` · `.Knowledges/32-index-and-source.md` |
| C++ / shader source index | `Plugins/AxonMCPs/AxonSource/` · `.Knowledges/32-index-and-source.md` |

## gotchas

### Graph layout (Agent must follow)

- Replace node → `animation.replace_anim_graph_node` / `blueprint.replace_blueprint_node` (keeps coordinates).
- Add near existing → pass `anchor_node` + `anchor_mode` (`at`|`right`|`below`).
- After local edits → `auto_layout` with `layout_mode=selected` or `new_only` (BP); AnimGraph same.
- Never use full-graph `layout_mode=all` on partitioned/comment-heavy graphs unless rebuilding the whole graph.
- Prefer `get_anim_node_positions` over dumping full get_nodes for layout.

- Core meta tools expand as `axon_*`; everything else is `{namespace}_query`.
- Default port is **9320** so Axon can coexist with Monolith on 9316.
- Sibling modules should use `LoadingPhase: Default` (AxonCore delays HTTP start until `OnPostEngineInit`, ensuring all Default-phase modules complete first).
- Claim a unique lowercase namespace; action names are `snake_case`.
- `bulk_fill` / `describe.schema` only work for namespaces that registered adapters; `describe.action_schema` works for any registered Action ParamSchema.
- Extension scaffolding is domain-agnostic: Core never ships built-in business recipes.
