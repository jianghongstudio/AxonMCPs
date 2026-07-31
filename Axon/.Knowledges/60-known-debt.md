# 已知坑与技术债

> **角色**：记录已识别风险与债务；解决后迁移到 [90-refactor-log.md](90-refactor-log.md) 并标记「已解决」。  
> **何时阅读**：规划改造、排查诡异行为时。  
> **相关源码**：全套件；见各条目  
> **相关文档**：[90-refactor-log.md](90-refactor-log.md)、[13-pie-sessions.md](13-pie-sessions.md)  
> **最后更新**：2026-08-01

## 基线快照

- **日期**：2026-08-01  
- **状态**：套件可用；下列来自源码结构与近期编译/拆分经验，非完整审计。

## 债务清单

### D1 — PIE 会话 API 分裂

- **位置**：`AxonEditorActions`（smoke/clip 创建）vs `AxonPieActions` / `AxonPieSession` / `AxonPieSmokeSession`（poll/stop/timeseries）
- **问题**：两套 manager；`poll_pie_smoke` 名称同时服务 smoke 与 timeseries，认知负担高。
- **影响**：改 poll/stop 或 session_id 格式时易漏一侧。
- **建议**：统一 session 门面（kind=smoke|timeseries|clip）。

### D2 — 地图门禁逻辑双份

- **位置**：`EnsureNoResidentPieWorldBeforeMapLoad` 在 EditorActions 与 PieActions 各有实现（挂钩不同 session manager）
- **问题**：策略漂移风险。
- **建议**：抽到共享 helper（具名 namespace 或 `AxonPieObject`）。

### D3 — Unity 匿名命名空间同名 helper（已缓解）

- **位置**：历史：`AxonPieActions` / `AxonPieSession` / `AxonPieSmokeSession` / EditorActions
- **问题**：Unity 合成 TU 后 C2011/C2084。
- **状态**：**已缓解**（`*Private` 具名 namespace + `AxonPieObject::FindPieWorld`）。
- **残余**：其它 Editor cpp 仍大量 `namespace { }`；新增同名符号仍可能炸。

### D4 — 项目 MCP 客户端默认可能指向 Monolith

- **位置**：仓库根 `.mcp.json` vs `Templates/.mcp.json.example`
- **问题**：用户以为连了 Axon，实际打到 9316。
- **建议**：文档醒目标注；可选双 server 配置示例。

### D5 — 无独立 Camera sibling

- **问题**：Camera 制作能力分散在 CameraBlueprint（非 Axon）+ RewindDebugger 采样 + Editor 捕获。
- **影响**：Agent 缺少单一 `camera_query` 入口。
- **建议**：若 3C 镜头 authoring 成为一等公民，再脚手架 `AxonCamera`。

### D6 — Action 面极大，文档易腐

- **问题**：animation/blueprint/gas 合计数百 Action；静态文档无法逐条维护。
- **建议**：权威列表以 `axon_discover(detail=true)` / `describe.action_schema` 为准；文档只维护工作流与边界。

### D7 — Docs/plans 引用可能缺失

- **位置**：部分源码注释引用 `Docs/plans/*.md`
- **问题**：当前 `Docs/` 未见 plans 目录。
- **状态**：待充实（确认是否在其它分支/未入库）。

## 已解决（保留痕迹）

| ID | 摘要 | 解决时间 |
|----|------|----------|
| D3 | PIE Unity 匿名命名空间冲突 | 2026-08-01 |
