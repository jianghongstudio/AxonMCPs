# 扩展 Cookbook

> **角色**：落地新 sibling / 新 Action / BulkFill adapter 的 Checklist。  
> **何时阅读**：要扩展 Axon 能力，且不想改 Core 时。  
> **相关源码**：`Docs/SIBLING_PLUGIN_GUIDE.md`、`Templates/ExtensionPlugin/`、`AxonExtensionScaffolder.*`  
> **相关文档**：[11-action-registry.md](11-action-registry.md)、[12-describe-bulk-fill.md](12-describe-bulk-fill.md)、[30-sibling-plugins.md](30-sibling-plugins.md)、[Docs/axon_guide.md](../Docs/axon_guide.md)  
> **最后更新**：2026-08-01

## A. 用 MCP 脚手架（推荐）

遵循 `Docs/axon_guide.md` 门禁：

1. 问用户是否需要 **推荐 Actions**。  
2. 若需要：`axon_research_extension_target` → 提案 → **等人确认**。  
3. `axon_create_extension`（先 `dry_run=true`）；确认后正式写入 `Plugins/AxonMCPs/<Name>/`。  
4. 关编辑器 → 编译 → 重启 → `axon_discover({namespace})`。

可选：`axon_list_extension_recipes` 查看外部 recipe JSON。

## B. 手工最小插件

1. `.uplugin`：依赖 `Axon`，模块 `Editor` + `PostEngineInit`。  
2. `Build.cs`：`AxonCore`、`UnrealEd`、`Json` 等。  
3. `StartupModule`：`FAxonToolRegistry::Get().RegisterAction(...)` + `FParamSchemaBuilder`。  
4. `ShutdownModule`：`FAxonCoreModule::IsAvailable()` 后 `UnregisterNamespace`。  
5. （可选）`FAxonBulkFillRegistry::RegisterAdapter`。

细节与代码骨架：[`Docs/SIBLING_PLUGIN_GUIDE.md`](../Docs/SIBLING_PLUGIN_GUIDE.md)。

## C. 新增单个 Action（已有 sibling）

- [ ] 选对 namespace（勿污染 `axon` / `editor` 除非真是跨域编辑器能力）  
- [ ] 写清 description（副作用、PIE、写盘）  
- [ ] ParamSchema 完整；复杂对象在描述中举例  
- [ ] Register + Unregister 对称  
- [ ] 若跨多个 cpp：helper 用 **具名** private namespace  
- [ ] 更新 [30-sibling-plugins.md](30-sibling-plugins.md) / [51-3c-workflows.md](51-3c-workflows.md)（若影响 3C 工作流）  
- [ ] 人类文档需要时更新 `Docs/3C_WORKFLOWS.md`

## D. BulkFill adapter

- [ ] 实现 BulkFill + Describe 回调  
- [ ] `RegisterAdapter(namespace, …)` 与模块 Shutdown 注销  
- [ ] 用 `dry_run=true` 验证  
- [ ] 更新 [12-describe-bulk-fill.md](12-describe-bulk-fill.md) adapter 表

## 禁止

- 未经用户确认编造业务 Action 列表（在「要推荐」路径上）。  
- Fork AxonCore 塞游戏逻辑。  
- 文件级 `using namespace XxxPrivate`（Unity 歧义）。
