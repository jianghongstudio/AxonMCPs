# AxonEditor Action 面

> **角色**：按源文件归纳 `editor.*`（及 Editor 注册的 alias），方便定位实现。  
> **何时阅读**：改编辑器侧 Action、查「哪个 cpp 注册了某 action」时。  
> **相关源码**：`Source/AxonEditor/Private/*Actions*.cpp`、`AxonEditorModule.cpp`  
> **相关文档**：[13-pie-sessions.md](13-pie-sessions.md)、[51-3c-workflows.md](51-3c-workflows.md)  
> **最后更新**：2026-08-01

## 注册入口

`FAxonEditorModule::StartupModule` 依次：

1. `FAxonPieActions::RegisterActions`
2. `FAxonEditorActions::RegisterActions`（含 smoke/clip/捕获/构建等）
3. `FAxonEditorMapActions` / `FAxonPieObjectActions` / `FAxonPieInputActions` / `FAxonStatActions`

## 按文件分组（非完整枚举）

### `AxonEditorActions.cpp`

构建与日志：`trigger_build`、`live_compile`、`get_build_errors`、`get_build_status`、`get_build_summary`、`search_build_output`、`get_compile_output`、`get_recent_logs`、`search_logs`、`tail_log`、`get_log_categories`、`get_log_stats`、`get_crash_context`、`run_console_command`

包：`list_dirty_packages`、`save_packages`

PIE smoke：`run_pie_smoke`、`capture_pie_movement_clip`、`list_errored_blueprints`

地图/harness：`create_nav_harness_map`、`author_map_settings`、`load_level`（部分可能在同文件其它 namespace helper 中）

捕获与检查：`capture_scene_preview`、`capture_sequence_frames`、`capture_anim_frames`、`capture_system_gif`、`import_texture`、`stitch_flipbook`、`delete_assets`、`get_viewport_info`、自动化与 `run_python` 等

实现拆分：`AxonEditorInspectActions.cpp`（材质/贴图 inspect）、`AxonEditorPreviewActions.cpp`（grid/overlay capture）

### `AxonPieActions.cpp`

`sample_pie_timeseries`（+ `animation` alias）、`poll_pie_smoke`、`stop_pie_smoke`、`start_pie`、`stop_pie`

### `AxonPieInputActions.cpp`

`pie_set_control_rotation`、`pie_inject_input_action`、`pie_possess_spectator_free`

### `AxonPieObjectActions.cpp`

`pie_get_object_properties`、`pie_call_function`

### `AxonEditorMapActions.cpp`

`create_empty_map`、`get_module_status`（以源码为准）

### `AxonStatActions.cpp`

`get_stat_group_values`

## 实现注意

- 大文件 `AxonEditorActions.cpp` 内已有具名 namespace：`AxonEditorPieSmoke`、`AxonEditorPackages` 等；新增 helper 优先具名，勿用匿名 namespace 堆同名符号。
- Smoke 创建仍在 EditorActions；poll/stop/timeseries 在 PieActions——改行为时两边都要核对。

## 待充实

- 从 `RegisterAction` 自动生成的完整 action 表（可脚本导出）。
