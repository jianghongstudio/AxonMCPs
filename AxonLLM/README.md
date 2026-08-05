# AxonLLM

Configurable **local LLM worker pool** for Axon Agents. MCP namespace: **`worker`** → tool `worker_query`.

Agent remains the orchestrator; workers compress evidence, draft Markdown, summarize logs, or promote drafts. They do **not** call other MCP actions.

## Enable

1. Plugin under `Plugins/AxonMCPs/AxonLLM/` (`EnabledByDefault` + project Plugins entry).
2. Compile editor / restart.
3. Project Settings → **Plugins → Axon LLM** (or click the editor status-bar **Axon MCP** chip → 打开 Axon LLM 设置). Busy workers show as `Axon · {model}` on that chip.
4. **默认知识库插件**：下拉列出已 `RegisterAll` 的知识包（`FAxonKnowledgeRegistry`；多包并存时只设一个兜底；`run` 仍可每次传 `kb_plugin` 覆盖）。
5. Point each worker’s `BaseUrl` at Ollama; use **刷新模型** to fill the Model dropdown from `/api/tags`.
6. Enable capabilities with Chinese checkboxes under **Worker|能力** (wire scope strings are for MCP only).

## Workers

- **Identity = array index** (`0`, `1`, …). No `worker_id`.
- **Order = priority**: `run` / `run_async` auto-picks the first enabled entry that allows the scope.
- Results report `worker_index` for diagnostics only.

| Index | Model (default) | Capabilities |
|-------|-----------------|--------------|
| `0` | `qwen3:14b` | summarize / draft / log / promote |
| `1` | `deepseek-coder:6.7b` | summarize / log |

## Actions

```
worker_query({ "action": "list" })
worker_query({ "action": "status" })
worker_query({ "action": "usage_summary", "days": 7 })

worker_query({
  "action": "run",
  "scope": "knowledge.summarize_raw",
  "kb_plugin": "AxonGaspKB",
  "paths": ["_manifest.json"]
})

worker_query({
  "action": "run",
  "scope": "knowledge.draft_topic",
  "kb_plugin": "AxonGaspKB",
  "topic": "10-motion-matching",
  "paths": ["choosers/....json"],
  "extra_instructions": "Focus on database selection"
})

worker_query({
  "action": "run",
  "scope": "log.summarize",
  "text": "...paste build error...",
  "paths": ["NextGame.log"]
})

worker_query({
  "action": "run",
  "scope": "knowledge.promote_draft",
  "kb_plugin": "AxonGaspKB",
  "topic": "10-motion-matching",
  "overwrite": false
})

worker_query({
  "action": "run_async",
  "scope": "knowledge.draft_topic",
  "kb_plugin": "AxonGaspKB",
  "topic": "20-overview",
  "paths": ["_manifest.json"]
})
# → { job_id, status: "queued" }
worker_query({ "action": "job_status", "job_id": "..." })
worker_query({ "action": "job_cancel", "job_id": "..." })
```

| Scope | Behavior |
|-------|----------|
| `knowledge.summarize_raw` | Short `output` (no disk write) |
| `knowledge.draft_topic` | Writes `Knowledge/_draft/{topic}.md` when `bDraftOnly=true` |
| `log.summarize` | Summarize Saved logs / inline `text` |
| `knowledge.promote_draft` | File move `_draft` → `Knowledge/` (no LLM) |

Async queue is **single-concurrent** to Ollama. `status` / `list` include `queue_depth`.

## Distill integration

After `{ns}_query` `extract_*` into `_raw/`:

1. `worker_query` / `run` or `run_async` → `_draft` Markdown  
2. Agent reviews / edits  
3. `knowledge.promote_draft` (or human) → formal `Knowledge/*.md`  
4. `{ns}_query` `search` / `read` accept  

Usage JSONL: `AxonLLM/Saved/usage/usage-YYYYMMDD.jsonl` (`usage_summary` aggregates).

See `Axon/.Knowledges/31-knowledge-distill.md`.

## P1 limits

- One Ollama job at a time via the async queue; sync `run` still blocks the caller.
- Running jobs cannot be cancelled reliably (`job_cancel` only queued).
- No MCP recursion from workers; no bulk-fill / search.rerank (P2).
