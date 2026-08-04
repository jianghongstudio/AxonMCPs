# AxonKnowledgeLib

Shared **mechanism** for Axon offline knowledge packs. Contains **no** business corpus.

## What it provides

| Surface | Purpose |
|---------|---------|
| `knowledge` MCP namespace | `preview_kb_names`, `scaffold_kb_plugin` |
| `FAxonKnowledgeCorpus` | Load/search/read `Knowledge/*.md` for a named KB plugin |
| `FAxonKnowledgeRegistration::RegisterAll` | Wire `route/search/read/list_topics` + `extract_*` onto a KB namespace |
| `FAxonKnowledgeExtract` | Invoke other Axon actions and write JSON under `Knowledge/_raw/` |
| `FAxonKnowledgeScaffold` | Create `Plugins/AxonMCPs/Axon{Project}KB/` thin siblings |

## Distill (product path)

When a user says **蒸馏本项目** / **distill this project**, Agents must follow:

`Docs/axon_guide.md` → **Distill current project**  
and the AI knowledge page: `Axon/.Knowledges/31-knowledge-distill.md`.

Human summary: `Axon/Docs/USER_GUIDE.md` §「项目蒸馏」。

## Consumers

- **AxonGaspKB** — example distilled pack (`gasp_kb`)
- **Axon{Project}KB** — per-project packs from `scaffold_kb_plugin`

Depends on plugin **Axon** (`AxonCore`). KB plugins depend on **Axon** + **AxonKnowledgeLib**.
