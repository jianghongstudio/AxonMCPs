# Action 注册表

> **角色**：说明 `FAxonToolRegistry` 的注册、执行与 ParamSchema 约定。  
> **何时阅读**：新增/改 Action、排查 unknown action、改 Shutdown 注销时。  
> **相关源码**：`Source/AxonCore/Public/AxonToolRegistry.h`、各模块 `*Actions.cpp` 的 `RegisterActions`  
> **相关文档**：[10-mcp-protocol.md](10-mcp-protocol.md)、[50-extension-cookbook.md](50-extension-cookbook.md)  
> **最后更新**：2026-08-01

## 模型

```
RegisterAction(Namespace, ActionName, Description, Handler, ParamSchema [, Category])
ExecuteAction(Namespace, ActionName, ParamsJson) → FAxonActionResult
UnregisterAction / UnregisterNamespace
```

- Namespace：小写短名（`editor`、`animation`、`gas`…）。
- ActionName：动词短语（`run_pie_smoke`、`scaffold_motion_matching_character`）。
- Handler：通常 `FAxonActionHandler::CreateStatic(&HandleXxx)`。
- ParamSchema：`FParamSchemaBuilder` 产出；供文档与 `describe.action_schema`。

## 生命周期

1. **Startup**：各模块/sibling 在 PostEngineInit 注册。
2. **Runtime**：HTTP `tools/call` → Registry。
3. **Shutdown**：先确认 `FAxonCoreModule::IsAvailable()`，再 `UnregisterNamespace`（避免 Core 已卸时崩溃）。

AxonEditor 示例：`FAxonEditorModule` 先 `FAxonPieActions::RegisterActions`，再 `FAxonEditorActions::RegisterActions` 等。

## 结果约定

`FAxonActionResult` 承载成功数据或错误字符串；可附加 error data（如 `errored_blueprints` 列表）。具体字段以头文件与现有 handler 为准。

## 实践要点

1. **描述写清副作用**：是否启动 PIE、是否写盘、是否 Live Coding。
2. **路径参数**：资产路径用 schema 的 AssetPath 类 helper（若 Builder 提供）；磁盘路径与 `/Game` 虚拟路径要在描述中区分。
3. **别名**：同一 handler 可注册到两个 namespace（如 `editor.sample_pie_timeseries` 与 `animation.sample_pie_timeseries`）。
4. **勿在 Core 堆业务**：新域 Action → sibling。

## 待充实

- Category 字段的客户端展示约定。
- 并发 / 游戏线程亲和：哪些 Action 必须在 GameThread（PIE 相关默认在编辑器线程上下文执行）。
