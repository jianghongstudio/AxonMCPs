# Sibling 插件与 Namespace 地图

> **角色**：描述 `Plugins/AxonMCPs/` 下各扩展插件的职责与 namespace，避免改错边界。  
> **何时阅读**：查「某个能力在哪个插件」、扩新 sibling、或梳理 3C 工具面时。  
> **相关源码**：`Plugins/AxonMCPs/*/Source/**`、各 `*.uplugin`  
> **相关文档**：[01-architecture.md](01-architecture.md)、[50-extension-cookbook.md](50-extension-cookbook.md)、[51-3c-workflows.md](51-3c-workflows.md)  
> **最后更新**：2026-08-04

## 目录约定

```
Plugins/AxonMCPs/
  Axon/                 ← Core + AxonEditor（本知识库宿主）
  AxonSource/
  AxonIndex/
  AxonAnimation/
  AxonBlueprint/
  AxonGAS/
  AxonConfig/
  AxonRewindDebugger/
  AxonKnowledgeLib/     ← 共享 KB Corpus/extract/scaffold（knowledge）
  AxonLLM/              ← 本地 LLM 工人池（worker）
  AxonGaspKB/           ← 离线 GASP 知识语料（gasp_kb）
  Axon{Project}KB/      ← knowledge.scaffold_kb_plugin 生成的按项目 KB
  <YourSibling>/        ← axon_create_extension 默认落点
```

## Namespace 地图（约数）

| Namespace | 插件 | 量级 | 3C 相关性 |
|-----------|------|------|-----------|
| `axon` | AxonCore | meta | 发现/状态/指南/脚手架 |
| `describe` / `bulk_fill` | AxonCore | 框架 | 反射读写 |
| `editor` | AxonEditor | ~50 | PIE、输入、捕获、构建日志 |
| `source` | AxonSource | 18 | Engine/Project C++ / Shader 索引查询 |
| `project` | AxonIndex | 11 | 资产 FTS 搜索 / 依赖 / GameplayTags |
| `animation` | AxonAnimation（+ 部分 Editor alias） | ~200 | 角色动画 authoring / PIE anim 采样 |
| `chooser` | AxonAnimation | ~10 | 选择表 |
| `blueprint` | AxonBlueprint | ~128 | MM 角色脚手架、图手术 |
| `gas` / `ui` | AxonGAS | ~135 + alias | 能力与输入绑定 |
| `config` | AxonConfig | ~7 | 设置/INI |
| `rewind_debugger` | AxonRewindDebugger | ~15 | 回放采样（含 CameraBP） |
| `knowledge` | AxonKnowledgeLib | 3 | KB 脚手架 + `list_kb_packs`（注册表索引） |
| `worker` | AxonLLM | 7 | 本地 LLM 工人（status/list/run/run_async/job_*/usage_summary） |
| `gasp_kb` | AxonGaspKB | ~11 | 离线 GASP 知识检索 + extract_*（不依赖 Content） |
| `{project}_kb` | Axon{Project}KB | ~11 | 按项目蒸馏的独立知识包 |

数量为源码静态扫描约数，以运行时 `axon_discover` 为准。

## 各 sibling 要点

### AxonSource

- C++ / Shader 源码 SQLite 索引；MCP namespace `source`（`source_query`）。
- 含 `read_source`、`search_source`、`get_signature`、`find_callers`、`trigger_reindex` 等 18 个 Action。
- DB 默认：`Plugins/AxonMCPs/AxonSource/Saved/EngineSource.db`。
- **启动不自动全量索引**；首次用前 `trigger_reindex`。Live Coding 后可 project 增量。
- 专页：[32-index-and-source.md](32-index-and-source.md)；插件 README：[`../AxonSource/README.md`](../AxonSource/README.md)。

### AxonIndex

- 项目资产深索引（BP / Material / GAS / Niagara 等）；MCP namespace `project`（`project_query`）。
- 含 `search`、`find_references`、`get_asset_details`、`refresh_assets` 等 11 个 Action。
- DB 默认：`Plugins/AxonMCPs/AxonIndex/Saved/ProjectIndex.db`。
- **编辑器启动后自动**全量/增量索引（Asset Registry ready）。
- 生成资产清理沙箱：`/Game/Tests/Axon/`。
- 专页：[32-index-and-source.md](32-index-and-source.md)；插件 README：[`../AxonIndex/README.md`](../AxonIndex/README.md)。

### AxonAnimation

- Locomotion 曲线、PoseSearch、ControlRig、ABP 图手术、Chooser。
- `animation.sample_pie_anim_instance`：live PIE AnimInstance 采样。
- BulkFill adapter：`animation`。

### AxonBlueprint

- 通用 BP CRUD / compile / spawn。
- **3C 关键**：`scaffold_motion_matching_character`、`scaffold_locomotion_input`、`apply_movement_preset` 等（`AxonMotionMatchingScaffoldActions`）。
- BulkFill adapter：`blueprint`。

### AxonGAS

- Ability / Effect / Attribute / ASC / Input / UI 绑定。
- **Control**：`setup_ability_input_binding`、`bind_ability_to_input`、`scaffold_input_binding_component`。
- BulkFill adapter：`gas`。

### AxonConfig

- 读/解释/diff DeveloperSettings 与 INI；dev-only 写入。

### AxonRewindDebugger

- 录制会话、Anim track 采样、`sample_camera_graph_result` / `sample_camera_watches`（可选依赖 CameraBlueprint）。

### AxonKnowledgeLib

- **无业务语料**；提供 `FAxonKnowledgeCorpus`、`FAxonKnowledgeRegistration::RegisterAll`、extract 写 `_raw`、以及 meta namespace `knowledge`。
- `RegisterAll` 同步登记到 `FAxonKnowledgeRegistry`；`UnregisterAll` 注销。消费者（AxonLLM 下拉等）按注册表索引，不靠插件名猜测。
- `knowledge.list_kb_packs` / `scaffold_kb_plugin` / `preview_kb_names`。
- Agent 触发：`axon_guide` → **Distill current project**（中文「蒸馏本项目」）。
- 专页：[31-knowledge-distill.md](31-knowledge-distill.md)；插件 README：[`../AxonKnowledgeLib/README.md`](../AxonKnowledgeLib/README.md)。

### AxonLLM

- 本地 LLM 工人池；MCP namespace `worker`（`worker_query`）。
- Actions：`status` / `list` / `run` / `run_async` / `job_status` / `job_cancel` / `usage_summary`。
- Scopes：`knowledge.summarize_raw`、`knowledge.draft_topic`（写 `_draft/`）、`log.summarize`、`knowledge.promote_draft`（文件升格，不调 LLM）。
- 配置：Project Settings → Plugins → Axon LLM；能力用中文勾选；Model 从 BaseUrl `/api/tags` 下拉。工人身份 = `Workers[]` 下标，无需 `worker_id`。
- Agent 调度；工人不递归调用其他 MCP；升格需显式 `promote_draft`。
- 插件 README：[`../AxonLLM/README.md`](../AxonLLM/README.md)；蒸馏联用见 [31-knowledge-distill.md](31-knowledge-distill.md)。

### AxonGaspKB

- 打包 Epic Game Animation Sample（GASP）蒸馏语料于 `Knowledge/*.md`；MCP namespace `gasp_kb`（`gasp_kb_query`）。
- 薄壳：Startup 调用 `FAxonKnowledgeRegistration::RegisterAll("gasp_kb", "AxonGaspKB")`。
- Actions：`route` / `search` / `read` / `list_topics` + `extract_*`。
- **不依赖** GASP Content / PoseSearch 运行时资产；可拷到任意带 Axon 的工程使用。
- 原始 dump 在 `Knowledge/_raw/`（不进主读路径）。
- 历史命名保留（不改为 `game_animation_sample_kb`）。

## 当前缺口

- **无独立 AxonCamera sibling**：镜头制作主要靠项目 CameraBlueprint + Rewind 采样 + Editor 捕获。

## 待充实

- 每 namespace「核心 Action 20 条」精选表（运行时 discover 导出）。
