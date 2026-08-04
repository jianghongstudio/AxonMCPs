# Axon

面向游戏 **3C（Character / Camera / Control）** 制作与验证的 Unreal Editor **MCP 核心**：在编辑器进程内托管 HTTP JSON-RPC（默认端口 **9320**），通过 `FAxonToolRegistry` 暴露可发现的 Action；同包 `AxonEditor` 提供 PIE 启动/时序采样/输入注入等编辑器侧能力。动画、蓝图、GAS 等领域工作放在 `Plugins/AxonMCPs/` 下的 sibling 插件，而不是塞进 Core。

> **AI 入口**：先读本 README 的知识索引，再按任务打开 [`.Knowledges/00-routing.md`](.Knowledges/00-routing.md)。改代码后按 [`.Knowledges/91-ai-maintenance.md`](.Knowledges/91-ai-maintenance.md) 同步文档。  
> **人类用户 / 扩展开发者**：优先读 [`Docs/USER_GUIDE.md`](Docs/USER_GUIDE.md)、[`Docs/3C_WORKFLOWS.md`](Docs/3C_WORKFLOWS.md)；Agent 速查仍可用 [`Docs/axon_guide.md`](Docs/axon_guide.md)。

## 模块速览

| 模块 | 类型 | 路径 | 职责 |
|------|------|------|------|
| **AxonCore** | Editor / PostEngineInit | [`Source/AxonCore/`](Source/AxonCore/) | HTTP MCP（`FAxonHttpServer`）、Action 注册表、`axon_*` meta、describe/bulk_fill 框架、扩展研究与脚手架 |
| **AxonEditor** | Editor / PostEngineInit | [`Source/AxonEditor/`](Source/AxonEditor/) | `editor.*`：构建/日志、地图、PIE smoke & timeseries、PIE 输入/对象、预览捕获、Stat |

Sibling（同级目录，依赖本插件）：`AxonAnimation`、`AxonBlueprint`、`AxonGAS`、`AxonConfig`、`AxonRewindDebugger`。总览见 [`../README.md`](../README.md)。

## 主数据流（摘要）

```
Cursor / MCP Client
        → HTTP POST http://localhost:9320/mcp  (JSON-RPC 2.0)
        → FAxonHttpServer::tools/call
        → axon_*  |  {ns}_query(action=…)  |  describe_query / bulk_fill_query
        → FAxonToolRegistry::ExecuteAction
        → 各模块 / sibling 的 RegisterAction handler
        → （可选）FAxonBulkFillRegistry adapter → ReflectionWalker / DryRun
```

细节见 [`.Knowledges/01-architecture.md`](.Knowledges/01-architecture.md)、[`.Knowledges/10-mcp-protocol.md`](.Knowledges/10-mcp-protocol.md)。

## 知识文档索引

面向 AI / 协作者的系列文档，位于 [`.Knowledges/`](.Knowledges/)。每篇只承担一类问题；**不要**把全部细节塞进 README。

| 文档 | 何时打开 | 一句话职责 |
|------|----------|------------|
| [00-routing.md](.Knowledges/00-routing.md) | 不确定该读哪篇时 | 按任务 / 症状路由 |
| [01-architecture.md](.Knowledges/01-architecture.md) | 需要总览或划模块边界时 | Core / Editor / sibling 边界与主数据流 |
| [02-glossary.md](.Knowledges/02-glossary.md) | 遇到陌生术语时 | 领域术语权威定义 |
| [10-mcp-protocol.md](.Knowledges/10-mcp-protocol.md) | 改 HTTP / 工具命名 / 客户端接入时 | JSON-RPC、端口、MCP 工具映射 |
| [11-action-registry.md](.Knowledges/11-action-registry.md) | 注册/注销 Action、改 ParamSchema 时 | `FAxonToolRegistry` 约定 |
| [12-describe-bulk-fill.md](.Knowledges/12-describe-bulk-fill.md) | 改反射写入 / schema 描述时 | describe / bulk_fill / dry_run |
| [13-pie-sessions.md](.Knowledges/13-pie-sessions.md) | 改 PIE smoke / timeseries / poll 时 | 异步 PIE 会话模型 |
| [30-sibling-plugins.md](.Knowledges/30-sibling-plugins.md) | 扩 sibling 或查 namespace 地图时 | 插件族与 namespace 边界 |
| [40-editor-actions.md](.Knowledges/40-editor-actions.md) | 改 AxonEditor Action 面时 | `editor.*` 按文件分组 |
| [50-extension-cookbook.md](.Knowledges/50-extension-cookbook.md) | 要落地扩展插件时 | 脚手架 Checklist |
| [51-3c-workflows.md](.Knowledges/51-3c-workflows.md) | 做 3C 验证 / Agent 编排时 | Character/Camera/Control 工作流 |
| [60-known-debt.md](.Knowledges/60-known-debt.md) | 评估风险 / 踩坑时 | 已知坑与技术债 |
| [90-refactor-log.md](.Knowledges/90-refactor-log.md) | 了解改造历史 / 记决策时 | 改造决策日志 |
| [91-ai-maintenance.md](.Knowledges/91-ai-maintenance.md) | 改完代码要同步文档时 | AI 协作与维护规则 |

## 人类可读文档（Docs/）

| 文档 | 受众 |
|------|------|
| [Docs/USER_GUIDE.md](Docs/USER_GUIDE.md) | 使用者：安装、连 MCP、常用工具 |
| [Docs/3C_WORKFLOWS.md](Docs/3C_WORKFLOWS.md) | 3C 策划/TA/程序：推荐操作链 |
| [Docs/axon_guide.md](Docs/axon_guide.md) | Agent 英文速查（onboarding / recipes / errors） |
| [Docs/SPEC_CORE.md](Docs/SPEC_CORE.md) | Core 规格摘要 |
| [Docs/SIBLING_PLUGIN_GUIDE.md](Docs/SIBLING_PLUGIN_GUIDE.md) | 扩展插件手工接入 |

## 改造状态

| 项 | 状态 |
|----|------|
| 知识文档框架 | 已搭建（骨架 + 路由 + 3C 工作流） |
| HTTP MCP + ToolRegistry | 已落地（默认 9320） |
| describe / bulk_fill 框架 | 已落地（animation / blueprint / gas adapter） |
| AxonEditor PIE smoke / timeseries | 已落地（会话模型仍分裂，见债务） |
| Sibling：Animation / Blueprint / GAS / Config / RewindDebugger | 已落地 |
| Unity 匿名命名空间冲突（PIE helpers） | 已缓解（具名 `*Private` namespace） |
| 项目根 `.mcp.json` 指向 Axon | 已落地（`http://localhost:9320/mcp`；Monolith 已禁用） |
| 独立 AxonCamera sibling | 无（Camera 经 RewindDebugger + CameraBlueprint 可选路径） |

当前阶段摘要：Axon 是 3C 向 MCP **总线 + 编辑器 PIE 验证核**；领域 authoring 在 sibling。见 [01-architecture.md](.Knowledges/01-architecture.md)、[51-3c-workflows.md](.Knowledges/51-3c-workflows.md)。
