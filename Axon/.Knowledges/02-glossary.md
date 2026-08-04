# 术语表

> **角色**：Axon / 3C MCP 领域术语的权威短定义。  
> **何时阅读**：文档或源码中出现陌生词时。  
> **相关源码**：全套件  
> **相关文档**：[01-architecture.md](01-architecture.md)、[Docs/USER_GUIDE.md](../Docs/USER_GUIDE.md)  
> **最后更新**：2026-08-04

| 术语 | 定义 |
|------|------|
| **3C** | Character / Camera / Control：角色表现、镜头、操控输入；Axon 套件的产品焦点。 |
| **Axon** | 本插件核心（`Axon.uplugin`）：HTTP MCP + Registry；含模块 AxonCore、AxonEditor。 |
| **AxonMCPs** | `Plugins/AxonMCPs/` 目录：Core + 全部 sibling 的安置约定。 |
| **Sibling / 扩展插件** | 依赖 Axon、自行 `RegisterAction` 的 Editor 插件（如 AxonAnimation）。 |
| **项目蒸馏 / Distill** | 把当前工程抽成独立离线 KB 插件（`Axon{Project}KB`）：scaffold → extract `_raw` → Agent 写 `Knowledge/*.md`。 |
| **KB 插件 / AxonXxxKB** | 仅含某工程蒸馏语料的 sibling；一项目一插件一 namespace；可单独拷贝启停。 |
| **AxonKnowledgeLib** | 蒸馏机制库（Corpus / RegisterAll / extract / `scaffold_kb_plugin`）；不含业务语料。 |
| **gasp_kb** | AxonGaspKB 的历史 namespace：Epic GASP 离线知识示例包。 |
| **AxonIndex / project** | 项目 UAsset 深索引（FTS + 引用图）；MCP `project_query`；启动后自动索引。 |
| **AxonSource / source** | Engine+Project C++/Shader 符号索引；MCP `source_query`；首次需 `trigger_reindex`。 |
| **Action** | 注册表内 `(namespace, action)` → handler + ParamSchema；内部执行单位。 |
| **Namespace** | Action 命名空间（如 `editor`、`animation`、`gas`）；MCP 侧常映射为 `{ns}_query`。 |
| **MCP 工具名** | 客户端可见名：`axon_discover`、`editor_query` 等；与内部 `namespace.action` 不同。 |
| **ParamSchema** | 用 `FParamSchemaBuilder` 声明的参数形状；可供 `describe.action_schema` 查询。 |
| **discover** | `axon.discover`：枚举 namespace / action（可过滤、分页、detail）。 |
| **guide** | `axon.guide`：返回 `Docs/axon_guide.md` 片段 + registry overlay。 |
| **describe** | 反射/schema 描述命名空间：`describe.schema` / `list_targets` / `action_schema`。 |
| **bulk_fill** | 按 JSON tree 反射写入资产；通常先 `dry_run=true`。 |
| **dry_run** | 只验证/报告、不落盘或不全量提交副作用的模式。 |
| **Adapter** | `FAxonBulkFillRegistry` 上 per-namespace 的 BulkFill/Describe 回调。 |
| **PIE** | Play-In-Editor；Axon 多异步启动并轮询，避免堵死游戏线程 HTTP。 |
| **PIE smoke** | `run_pie_smoke` / `capture_pie_movement_clip`：短时 PIE + 日志/Anim 采样（及可选帧捕获）。 |
| **timeseries** | `sample_pie_timeseries`：对目标 Actor/组件/AnimInstance 做点分路径时序采样 + provocation。 |
| **provocation** | 定时触发的控制侧动作：转视角、AddMovementInput、Jump、console 等。 |
| **on_compile_errors** | PIE 前对 `BS_Error` Blueprint 的策略：`refuse`（默认）或 `suppress`。 |
| **Monolith** | 同仓库另一套 Editor MCP（常见 9316）；与 Axon 端口/工具面分离。 |
| **哨兵文件** | `Axon/Saved/.axon_running`：记录 pid/port/version，便于外部探测（以源码为准）。 |

## 待充实

- 完整错误码表（与 `FAxonJsonUtils` / ActionResult 对齐）。
- Recipe JSON 字段权威列表（见 `Templates/ExtensionRecipes/README.md`）。
