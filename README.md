# AxonMCPs

面向游戏 **3C（Character / Camera / Control）** 制作流程的 Unreal Editor **MCP** 插件族：在编辑器内提供 HTTP JSON-RPC 总线与按域拆分的 Action 注册表，供 Cursor / Agent 驱动脚手架、PIE 验证、动画与 GAS authoring。

| 插件 | 角色 |
|------|------|
| **[Axon](Axon/)** | 核心：HTTP MCP、`FAxonToolRegistry`、discover/guide、describe/bulk_fill；同包 `AxonEditor` 承载 PIE / 编辑器 Action |
| [AxonSource](AxonSource/) | Engine/Project C++ 与 Shader 源码索引（`source_query`；首次 `trigger_reindex`） |
| [AxonIndex](AxonIndex/) | 项目资产 SQLite+FTS / 引用图（`project_query`；启动后自动索引） |
| [AxonAnimation](AxonAnimation/) | 动画 / Chooser / PoseSearch / Locomotion authoring |
| [AxonBlueprint](AxonBlueprint/) | Blueprint 图手术 + Motion Matching 角色脚手架 |
| [AxonGAS](AxonGAS/) | Gameplay Ability System authoring / inspect / 输入绑定 |
| [AxonConfig](AxonConfig/) | INI / DeveloperSettings 查询与 dev 写入 |
| [AxonRewindDebugger](AxonRewindDebugger/) | Rewind Debugger 会话与 Anim/Camera 采样 |
| [AxonKnowledgeLib](AxonKnowledgeLib/) | 项目蒸馏机制：`knowledge.scaffold_kb_plugin`、Corpus/extract（无业务语料） |
| [AxonGaspKB](AxonGaspKB/) | GASP 离线知识示例（`gasp_kb`）；形态范本 `Axon{Project}KB` |
| `Axon{Project}KB/` | 由蒸馏生成的按工程独立知识包（可单独拷贝） |

**索引**：资产用 Index（`project_query`），C++/Shader 用 Source（`source_query`）。对照见 [`Axon/.Knowledges/32-index-and-source.md`](Axon/.Knowledges/32-index-and-source.md)。

**蒸馏**：对 Agent 说「蒸馏本项目」→ 脚手架独立 KB 插件 → extract → 蒸馏 Markdown。详见 [`Axon/.Knowledges/31-knowledge-distill.md`](Axon/.Knowledges/31-knowledge-distill.md) 与 [`Axon/Docs/USER_GUIDE.md`](Axon/Docs/USER_GUIDE.md) §项目蒸馏。

> **AI 入口**：先读 [`Axon/README.md`](Axon/README.md) 知识索引，再按任务打开 [`Axon/.Knowledges/00-routing.md`](Axon/.Knowledges/00-routing.md)。  
> **人类用户 / 开发者**：见 [`Axon/Docs/USER_GUIDE.md`](Axon/Docs/USER_GUIDE.md) 与 [`Axon/Docs/3C_WORKFLOWS.md`](Axon/Docs/3C_WORKFLOWS.md)。

默认 MCP 地址：`http://localhost:9320/mcp`（示例配置见 [`Axon/Templates/.mcp.json.example`](Axon/Templates/.mcp.json.example)）。
