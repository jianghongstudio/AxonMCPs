# PIE 会话模型

> **角色**：说明 AxonEditor 异步 PIE smoke / timeseries 的生命周期、门禁与相关 Action。  
> **何时阅读**：改 smoke/timeseries、poll/stop、地图加载门禁、编译错误策略时。  
> **相关源码**：`AxonPieActions.cpp`、`AxonPieSession.*`、`AxonPieSmokeSession.*`、`AxonEditorActions.cpp`（`run_pie_smoke` / `capture_pie_movement_clip`）、`AxonPieObject.*`  
> **相关文档**：[40-editor-actions.md](40-editor-actions.md)、[51-3c-workflows.md](51-3c-workflows.md)、[60-known-debt.md](60-known-debt.md)  
> **最后更新**：2026-08-01

## 为什么异步

旧式「在 handler 里 pump 编辑器直到 PIE ready」会重入 `UWorld::Tick` 并拖死游戏线程上的 HTTP。现行模型：**启动 PIE + 注册 session 后立即返回**；真实帧由编辑器 tick 推进；客户端用 `poll_pie_smoke` 拉进度。

## 两套会话

| 会话 | 创建 Action | Manager | 典型用途 |
|------|-------------|----------|----------|
| Smoke / Clip | `run_pie_smoke`、`capture_pie_movement_clip` | `FPieSmokeSessionManager` | 日志 marker、AnimInstance 变量、可选视口帧、profiling |
| Timeseries | `sample_pie_timeseries` | `FAxonPieSessionManager` | 点分路径采样 + timed provocations |

**共享轮询名**：`poll_pie_smoke` / `stop_pie_smoke`（由 `FAxonPieActions` 注册）。历史命名兼容 smoke；timeseries 也走同名 poll/stop——实现上需按 session_id 路由到正确 manager（以源码为准；分裂本身记入债务）。

## 关键 Action

| Action | 说明 |
|--------|------|
| `start_pie` / `stop_pie` | 简单启停 in-viewport PIE |
| `run_pie_smoke` | 异步 smoke |
| `capture_pie_movement_clip` | smoke + 定时截帧 |
| `sample_pie_timeseries` | 时序采样（亦注册到 `animation`） |
| `poll_pie_smoke` / `stop_pie_smoke` | 轮询 / 停止 |
| `list_errored_blueprints` | PIE 前只读扫描 |

辅助：`pie_inject_input_action`、`pie_set_control_rotation`、`pie_get_object_properties`、`pie_call_function`、`run_console_command`。

## 门禁

### 编译错误 Blueprint

引擎在 PIE 前可能对 `BS_Error && bDisplayCompilePIEWarning` 弹模态，卡住 MCP。策略参数 `on_compile_errors`：

- `refuse`（默认）：返回 errored 列表，不启动。
- `suppress`：`GIsRunningUnattendedScript` 护栏绕过模态（慎用）。

### 地图加载与 resident PIE

`LoadLevel` 时若仍有 PIE World，会触发 World Memory Leaks。共享逻辑：`EnsureNoResidentPieWorldBeforeMapLoad`（EditorActions 与 PieActions 各有一份策略实现，分别挂钩不同 session manager）——先拒绝仍有 session 的情况，否则驱动 teardown + GC。

## World 查找

优先复用 `AxonPieObject::FindPieWorld()` / `FAxonEditorActions::FindActivePieWorld()`，避免各 cpp 再复制一份匿名 `FindActivePieWorld`（Unity 冲突史，见债务）。

## Unity 约定

PIE helper 使用具名 namespace：`AxonPieActionsPrivate`、`AxonPieSessionPrivate`、`AxonPieSmokeSessionPrivate`；成员函数内 `using namespace`，**不要**文件级 using（Unity 下会歧义）。

## 待充实

- 统一 session API（单一 manager + kind 字段）的设计草案。
- smoke report JSON 字段权威表（lifecycle / log_groups / profiling）。
