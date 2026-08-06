# 问题路由（AI 第二入口）

> **角色**：按任务 / 症状把读者导向正确知识文档，避免通读全库。  
> **何时阅读**：不确定该打开哪篇 `.Knowledges` 文档时；接到改造任务后的第二步（第一步是 README 索引）。  
> **相关源码**：全套件 `Plugins/AxonMCPs/`（本文件不绑定单一路径）  
> **相关文档**：[README.md](../README.md)、[91-ai-maintenance.md](91-ai-maintenance.md)、[Docs/USER_GUIDE.md](../Docs/USER_GUIDE.md)  
> **最后更新**：2026-08-04

## 使用方式

1. 在下方「按任务」或「按症状」表中找到最接近的一行。
2. 打开对应文档；需要术语时再开 [02-glossary.md](02-glossary.md)。
3. 改代码后按 [91-ai-maintenance.md](91-ai-maintenance.md) 回写文档。

## 按任务路由

| 任务 | 优先打开 | 可能还需要 |
|------|----------|------------|
| 理解套件整体 / 划 Core·Editor·sibling 边界 | [01-architecture.md](01-architecture.md) | [30-sibling-plugins.md](30-sibling-plugins.md) |
| 查术语 | [02-glossary.md](02-glossary.md) | — |
| 改 HTTP 端口 / JSON-RPC / MCP 工具名映射 | [10-mcp-protocol.md](10-mcp-protocol.md) | [Docs/SPEC_CORE.md](../Docs/SPEC_CORE.md) |
| 注册 / 注销 Action、改 ParamSchema | [11-action-registry.md](11-action-registry.md) | [50-extension-cookbook.md](50-extension-cookbook.md) |
| 改 describe / bulk_fill / dry_run / ReflectionWalker | [12-describe-bulk-fill.md](12-describe-bulk-fill.md) | [01-architecture.md](01-architecture.md) |
| 改 PIE smoke / timeseries / poll / stop | [13-pie-sessions.md](13-pie-sessions.md) | [51-3c-workflows.md](51-3c-workflows.md)、[40-editor-actions.md](40-editor-actions.md) |
| 查 sibling namespace 或扩扩展插件 | [30-sibling-plugins.md](30-sibling-plugins.md) → [50-extension-cookbook.md](50-extension-cookbook.md) | [Docs/SIBLING_PLUGIN_GUIDE.md](../Docs/SIBLING_PLUGIN_GUIDE.md) |
| 改 `editor.*` Action 面（非 PIE 会话内核） | [40-editor-actions.md](40-editor-actions.md) | [13-pie-sessions.md](13-pie-sessions.md) |
| 脚手架新 sibling / recipe | [50-extension-cookbook.md](50-extension-cookbook.md) | [Docs/axon_guide.md](../Docs/axon_guide.md) |
| 蒸馏本项目 / 离线 KB 包 / gasp_kb | [31-knowledge-distill.md](31-knowledge-distill.md) | [30-sibling-plugins.md](30-sibling-plugins.md)、[Docs/USER_GUIDE.md](../Docs/USER_GUIDE.md)、[Docs/axon_guide.md](../Docs/axon_guide.md) |
| 搜项目资产 / 引用图 / GameplayTag | [32-index-and-source.md](32-index-and-source.md) | [30-sibling-plugins.md](30-sibling-plugins.md) |
| 查引擎/工程 C++、调用方、include | [32-index-and-source.md](32-index-and-source.md) | [30-sibling-plugins.md](30-sibling-plugins.md) |
| 编排 3C 验证（角色移动 / 输入 / 相机回放） | [51-3c-workflows.md](51-3c-workflows.md) | [Docs/3C_WORKFLOWS.md](../Docs/3C_WORKFLOWS.md) |
| 评估风险 / Unity 冲突 / 会话分裂 | [60-known-debt.md](60-known-debt.md) | [90-refactor-log.md](90-refactor-log.md) |
| 记决策 / 查改造史 | [90-refactor-log.md](90-refactor-log.md) | [60-known-debt.md](60-known-debt.md) |
| 同步知识库 | [91-ai-maintenance.md](91-ai-maintenance.md) | [README.md](../README.md) |
| 写给人类的接入说明 | [Docs/USER_GUIDE.md](../Docs/USER_GUIDE.md) | [Docs/3C_WORKFLOWS.md](../Docs/3C_WORKFLOWS.md) |

## 按症状路由

| 症状 / 现象 | 优先打开 | 备注 |
|-------------|----------|------|
| Connection refused / 9320 无响应 | [10-mcp-protocol.md](10-mcp-protocol.md)、[Docs/USER_GUIDE.md](../Docs/USER_GUIDE.md) | 编辑器未开、插件关、端口改、kill-switch |
| `axon_discover` 缺 namespace | [30-sibling-plugins.md](30-sibling-plugins.md) | sibling 未启用或未加载 |
| Unknown action / 参数校验失败 | [11-action-registry.md](11-action-registry.md) | `describe.action_schema` |
| PIE 编译错误弹窗卡死 MCP | [13-pie-sessions.md](13-pie-sessions.md) | `on_compile_errors` / `list_errored_blueprints` |
| LoadLevel 时报 World Memory Leaks | [13-pie-sessions.md](13-pie-sessions.md) | `EnsureNoResidentPieWorldBeforeMapLoad` |
| `poll_pie_smoke` 找不到 session | [13-pie-sessions.md](13-pie-sessions.md) | smoke 与 timeseries 两套 manager |
| Unity：匿名命名空间符号重定义 | [60-known-debt.md](60-known-debt.md) | 用 `*Private` 具名 namespace |
| bulk_fill 无 adapter / dry_run 无报告 | [12-describe-bulk-fill.md](12-describe-bulk-fill.md) | 仅 animation/blueprint/gas 已注册 |
| Camera 采样为空 | [51-3c-workflows.md](51-3c-workflows.md) | 需 CameraBlueprint + RewindDebugger |
| 根 `.mcp.json` 连错服务器 | [Docs/USER_GUIDE.md](../Docs/USER_GUIDE.md) | Monolith 9316 vs Axon 9320 |
| 说「蒸馏」未出 AxonXxxKB | [31-knowledge-distill.md](31-knowledge-distill.md) | 须 `scaffold_kb_plugin` + 编译重启 |
| `knowledge` / `{ns}_kb` 未知 | [31-knowledge-distill.md](31-knowledge-distill.md)、[30-sibling-plugins.md](30-sibling-plugins.md) | 启用 AxonKnowledgeLib / 对应 KB 插件 |
| `source` 空索引 / 无符号 | [32-index-and-source.md](32-index-and-source.md) | 先 `trigger_reindex` |
| `project` 搜不到新资产 | [32-index-and-source.md](32-index-and-source.md) | `refresh_assets` 或等启动增量 |

## 模块 → 文档速查

| 源码位置 | 默认文档 |
|----------|----------|
| `Source/AxonCore/` | [10-mcp-protocol.md](10-mcp-protocol.md)、[11-action-registry.md](11-action-registry.md)、[12-describe-bulk-fill.md](12-describe-bulk-fill.md) |
| `Source/AxonEditor/`（PIE） | [13-pie-sessions.md](13-pie-sessions.md)、[40-editor-actions.md](40-editor-actions.md) |
| `../AxonAnimation/` 等 sibling | [30-sibling-plugins.md](30-sibling-plugins.md)、[51-3c-workflows.md](51-3c-workflows.md) |
| `../AxonKnowledgeLib/`、`../AxonGaspKB/`、`../Axon*KB/` | [31-knowledge-distill.md](31-knowledge-distill.md)、[30-sibling-plugins.md](30-sibling-plugins.md) |
| `../AxonIndex/`、`../AxonSource/` | [32-index-and-source.md](32-index-and-source.md)、[30-sibling-plugins.md](30-sibling-plugins.md) |
| `Docs/` | 人类文档；Agent 速查 `axon_guide.md` |
