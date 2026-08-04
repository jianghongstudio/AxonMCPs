# Sibling 插件与 Namespace 地图

> **角色**：描述 `Plugins/AxonMCPs/` 下各扩展插件的职责与 namespace，避免改错边界。  
> **何时阅读**：查「某个能力在哪个插件」、扩新 sibling、或梳理 3C 工具面时。  
> **相关源码**：`Plugins/AxonMCPs/*/Source/**`、各 `*.uplugin`  
> **相关文档**：[01-architecture.md](01-architecture.md)、[50-extension-cookbook.md](50-extension-cookbook.md)、[51-3c-workflows.md](51-3c-workflows.md)  
> **最后更新**：2026-08-01

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

数量为源码静态扫描约数，以运行时 `axon_discover` 为准。

## 各 sibling 要点

### AxonSource

- C++ / Shader 源码 SQLite 索引；MCP namespace `source`（`source_query`）。
- 含 `read_source`、`search_source`、`get_signature`、`trigger_reindex` 等 18 个 Action。
- DB 默认：`Plugins/AxonMCPs/AxonSource/Saved/EngineSource.db`。

### AxonIndex

- 项目资产深索引（BP / Material / GAS / Niagara 等）；MCP namespace `project`（`project_query`）。
- 含 `search`、`find_references`、`get_asset_details`、`refresh_assets` 等 11 个 Action。
- DB 默认：`Plugins/AxonMCPs/AxonIndex/Saved/ProjectIndex.db`。
- 生成资产清理沙箱：`/Game/Tests/Axon/`。

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

## 当前缺口

- **无独立 AxonCamera sibling**：镜头制作主要靠项目 CameraBlueprint + Rewind 采样 + Editor 捕获。

## 待充实

- 每 namespace「核心 Action 20 条」精选表（运行时 discover 导出）。
