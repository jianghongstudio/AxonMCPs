# 项目蒸馏与独立 KB 插件

> **角色**：说明 Axon 如何把任意工程蒸馏成可离线查询的独立知识包插件。  
> **何时阅读**：用户说「蒸馏本项目」/ distill；实现或排查 `knowledge` / `AxonXxxKB` / `gasp_kb`。  
> **相关源码**：`Plugins/AxonMCPs/AxonKnowledgeLib/`、`AxonGaspKB/`、`Axon/Docs/axon_guide.md`（Distill recipe）  
> **相关文档**：[30-sibling-plugins.md](30-sibling-plugins.md)、[50-extension-cookbook.md](50-extension-cookbook.md)、[Docs/USER_GUIDE.md](../Docs/USER_GUIDE.md)  
> **最后更新**：2026-08-04

## 一句话

在任意已装 **Axon + AxonKnowledgeLib** 的工程里，对 Agent 说「蒸馏本项目」→ 自动脚手架独立 sibling `Axon{Project}KB` → 编译重启 → extract 证据到 `_raw` → Agent 蒸馏 `Knowledge/*.md` → 用 `{project}_kb_query` 离线问答（不依赖源 Content）。

## 能力分层

| 层 | 插件 | Namespace | 职责 |
|----|------|-----------|------|
| 机制库 | **AxonKnowledgeLib** | `knowledge` | 命名推导、`scaffold_kb_plugin`、Corpus/extract、`FAxonKnowledgeRegistry`；**无业务语料** |
| 示例包 | **AxonGaspKB** | `gasp_kb` | Epic Game Animation Sample 已蒸馏语料（历史命名保留） |
| 项目包 | **Axon{Project}KB** | `{snake}_kb` | 每个被蒸馏工程一个独立插件；可单独拷贝/启停 |

知识包在 `StartupModule` 调用 `FAxonKnowledgeRegistration::RegisterAll` 时**主动注册**到 `FAxonKnowledgeRegistry`（不是靠文件夹名猜测）。查询已加载包：`knowledge_query` → `list_kb_packs`。AxonLLM「默认知识库插件」下拉也读此注册表。

**一项目 = 一插件 = 一 namespace。** 禁止把多工程语料塞进同一个宿主 Pack。

## Agent 强制流程

触发语（须走本流程，不可只 extract 不建插件）：

- 中文：`蒸馏本项目` / `蒸馏` + 工程名  
- 英文：`distill this project` / `distill` + project name  

步骤（权威英文版见 `Docs/axon_guide.md` → **Distill current project**）：

1. `knowledge_query` → `preview_kb_names`（或读 `axon_status.project_name`）  
2. `scaffold_kb_plugin`（先 `dry_run=true`）写入 `Plugins/AxonMCPs/Axon{Project}KB/`  
3. 关编辑器 → UBT → 重启（新 `.uplugin` 必需）  
4. `{ns}_query` → `extract_*` / `extract_bundle` → `Knowledge/_raw/`  
5. 撰写 `Knowledge/*.md`（C++ **不**自动生成全书；深度标杆对齐 AxonGaspKB）  
   - **可选（AxonLLM）**：`worker_query` → `run` / `run_async`：`knowledge.summarize_raw` 压缩证据；`knowledge.draft_topic` 写入 `Knowledge/_draft/*.md`；审阅后 `knowledge.promote_draft` 升格为正式 topic。长任务用 `run_async` + `job_status`。  
   - 未启用 AxonLLM 时，仍由 Agent 直接撰写正式 md。  
6. `{ns}_query` → `search` / `read` / `list_topics` 验收  

特例：GASP 已存在 → **不要**再 scaffold `AxonGameAnimationSampleKB`；继续用 `AxonGaspKB` / `gasp_kb`。

## 命名规则

| 输入 `project_name` | `plugin_name` | `namespace` | MCP 工具 |
|---------------------|---------------|-------------|---------|
| 任意（例 `FooBar`） | `AxonFooBarKB` | `foo_bar_kb` | `foo_bar_kb_query` |
| GASP（历史） | `AxonGaspKB` | `gasp_kb` | `gasp_kb_query` |

## 查询与提取 Actions

每个 KB 插件（经 `FAxonKnowledgeRegistration::RegisterAll`）通常注册：

| 类 | Actions |
|----|---------|
| 查询 | `route` / `search` / `read` / `list_topics` |
| 提取 | `extract_write` / `extract_invoke` / `extract_state_machines` / `extract_anim_graph_overview` / `extract_chooser` / `extract_config_ddcvars` / `extract_bundle` |

- 查询只读 `Knowledge/*.md`（跳过 `_raw/`）。  
- 提取经 `ExecuteAction` 调用 animation/chooser/config 等，JSON 写入本插件 `Knowledge/_raw/`。  
- 离线问答：**不需要**源工程 Content；只需目标工程启用该 KB 插件 + AxonKnowledgeLib。

## 诚实边界

- 新插件必须 **编译 + 重启** 后 namespace 才出现；不是零编译黑盒。  
- Markdown 蒸馏由 **Agent** 完成（可委派 AxonLLM 出 `_draft`），不是一键全书生成。  
- 本地工人：启用 **AxonLLM** 后用 `worker_query`；配置见 [`../AxonLLM/README.md`](../AxonLLM/README.md)。  
- KB 包索引：`RegisterAll` 会写入 `FAxonKnowledgeRegistry`；可 `knowledge_query` → `list_kb_packs` 查看（AxonLLM 默认插件下拉同源）。  

- 是否把 `AxonXxxKB` 拷到其他工程由**人**决定（启停/拷贝粒度 = 插件）。  
- KB 脚手架请用 `knowledge.scaffold_kb_plugin`，**不要**用 `axon_create_extension`（后者不会正确接线 KnowledgeLib / Knowledge 模板）。

## 快速命令

```
knowledge_query({ "action": "preview_kb_names" })
knowledge_query({ "action": "scaffold_kb_plugin", "dry_run": true })
knowledge_query({ "action": "scaffold_kb_plugin" })
# close editor → UBT → relaunch
{ns}_query({ "action": "extract_bundle", "jobs": [ ... ] })
# optional local draft (AxonLLM):
worker_query({ "action": "status" })
worker_query({ "action": "run", "scope": "knowledge.draft_topic", "kb_plugin": "AxonFooBarKB", "topic": "10-overview", "paths": ["_manifest.json"] })
worker_query({ "action": "run", "scope": "knowledge.promote_draft", "kb_plugin": "AxonFooBarKB", "topic": "10-overview" })
{ns}_query({ "action": "search", "query": "..." })
gasp_kb_query({ "action": "list_topics" })   # 示例包回归
```

## 维护勾选

- [ ] 改 scaffold / 命名 / extract API → 同步本文 + `axon_guide` Distill recipe + `USER_GUIDE` §蒸馏  
- [ ] 新增示例 KB 插件 → [30-sibling-plugins.md](30-sibling-plugins.md) namespace 表  
- [ ] 改 GASP 语料结构 → `AxonGaspKB/Knowledge/` 与 skill（若有）同步  
