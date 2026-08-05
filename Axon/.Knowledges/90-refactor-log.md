# 改造决策日志

> **角色**：按时间记录改造动机、方案、影响面；过程真相源。  
> **何时阅读**：了解「为什么现在是这样」；实质性改造后必须追加。  
> **相关源码**：随条目变化  
> **相关文档**：[60-known-debt.md](60-known-debt.md)、[README.md](../README.md)、[91-ai-maintenance.md](91-ai-maintenance.md)  
> **最后更新**：2026-08-05

## 当前阶段

| 字段 | 值 |
|------|----|
| 阶段 | MCP tools/list 与 Cursor 缓存竞态缓解 |
| 基线日期 | 2026-08-05 |
| 技术债索引 | [60-known-debt.md](60-known-debt.md) |

### 2026-08-05 — 延迟 MCP HTTP 启动，修复 Cursor 缺 `worker_query`

- **动机**：服务端 `tools/list` 已含 `worker_query`，Cursor 会话工具表没有；根因是 Core 在 sibling 注册前开端口，客户端缓存残缺列表。
- **方案**：`StartHttpServerIfEnabled` 挂到 `OnAllModuleLoadingPhasesComplete`（ticker 0 兜底）；`listChanged=true`。
- **影响面**：`AxonCoreModule`、`AxonHttpServer::HandleInitialize`
- **文档同步**：`60` D8、本条目
- **关联债务**：D8 已缓解；运行中热启 sibling 仍需重连（残余）

### 2026-08-04 — 按项目独立 KB 蒸馏

- **动机**：任意工程可蒸馏为可拷贝的离线知识包；一项目一插件，便于启停与隔离。
- **方案**：
  - sibling `AxonKnowledgeLib`：Corpus / RegisterAll / extract_* / `knowledge.scaffold_kb_plugin`
  - `AxonGaspKB` 薄壳接入 Lib；历史 namespace `gasp_kb` 保留
  - `axon_guide` 强制 Distill recipe；文档 `31-knowledge-distill.md` + USER_GUIDE §项目蒸馏
- **影响面**：新插件、AxonGaspKB、guide / Knowledges / USER_GUIDE
- **文档同步**：`31`、`30`、`00`、`02`、`50`、`91`、README、USER_GUIDE、AxonMCPs README
- **关联债务**：无新开；Markdown 全书仍由 Agent 撰写（非 C++ 一键生成）

### 2026-08-01 — 建立 `.Knowledges` 与人类 Docs

- **动机**：Axon 定位为游戏 3C MCP，需要与 AnimStateMachine 同级的 AI 知识库，以及给用户/开发者的接入与工作流文档。
- **方案**：
  - `Axon/README.md` + `.Knowledges/`（路由/架构/协议/PIE/sibling/3C/债务/维护）
  - `Docs/USER_GUIDE.md`、`Docs/3C_WORKFLOWS.md`
  - `Plugins/AxonMCPs/README.md` 套件入口
- **影响面**：仅文档
- **文档同步**：本条目、README 改造状态
- **关联债务**：D4/D5/D6 仍开放

### 2026-08-01 — PIE helpers 具名 namespace（Unity）

- **动机**：`Module.AxonEditor.cpp` Unity 合并导致 `FErroredBlueprintEntry` / `FindActivePieWorld` / `ResolveProvocationPawn` 等重定义。
- **方案**：
  - `AxonPieActionsPrivate` / `AxonPieSessionPrivate` / `AxonPieSmokeSessionPrivate`
  - SmokeSession 改用 `AxonPieObject::FindPieWorld()`
  - 函数作用域 `using namespace`，避免文件级 using
- **影响面**：`AxonEditor` PIE 相关 cpp
- **文档同步**：`13` / `60` D3
- **关联债务**：D3 标记已缓解；D1/D2 仍开放
