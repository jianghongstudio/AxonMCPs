# Axon 3C 工作流（用户版）

面向策划、TA、程序与 Agent 使用者：用 Axon MCP 做 **Character / Camera / Control** 相关制作与抽检。工具名以客户端的 `{namespace}_query` + `action` 为准；参数细节用 `describe_query` 查询。

接入与排错见 [USER_GUIDE.md](USER_GUIDE.md)。AI 内部编排说明见 [`.Knowledges/51-3c-workflows.md`](../.Knowledges/51-3c-workflows.md)。

---

## 开始前检查清单

1. 编辑器运行中，Axon 监听 **9320**（`axon_status`）。  
2. 需要的 sibling 已启用（动画 / 蓝图 / GAS / Rewind 等）。  
3. 目标地图与资产路径已知。  
4. 建议先跑 `list_errored_blueprints`，避免 PIE 被编译错误弹窗卡住。

---

## Character — 角色移动与动画

### A. 脚手架一条 Motion Matching / Locomotion 角色（authoring）

**适用：** 新建或补齐角色 BP / 输入 / Anim 侧约定。

1. `blueprint_query` → `scaffold_motion_matching_character`（及项目约定的 companion scaffold，如 locomotion input / anim values）。  
2. 按需 `animation_query`：PoseSearch / Chooser / ABP 图相关 action。  
3. `validate_animbp_variable_contract` 一类校验（若已注册）确认变量契约。  
4. `editor_query` → `save_packages`（确认 dirty 范围；可用 `list_dirty_packages` 预览）。

### B. 短时 PIE Smoke（逻辑回归）

**适用：** 「进 PIE 跑几秒，看 AnimInstance 变量 / 日志是否干净」。

1. `editor_query` → `list_errored_blueprints`。  
2. `run_pie_smoke`：  
   - `map`（可选）  
   - `duration`  
   - `sample_vars`（如 `GroundSpeed`、`bShouldMove`）  
   - `pawn_class` / `console_script` / `python_script`（可选）  
   - `on_compile_errors`: 默认 `refuse`  
3. 立即返回 `session_id` → 循环 `poll_pie_smoke`（可设 `include_samples`）。  
4. 结束：自动完成或 `stop_pie_smoke`。

### C. 移动 Clip（带截图）

**适用：** 给评审看一段移动表现。

1. `capture_pie_movement_clip`：设置 `output_path`、`capture_interval`、`view_target_actor`（可选）。  
2. `poll_pie_smoke` 直到 complete，收集帧路径与采样。

### D. 时序采样 + 操控刺激（provocation）

**适用：** 观察若干秒内变量曲线，并在指定时刻转向 / 加移动输入 / Jump。

1. `sample_pie_timeseries`（也出现在 `animation_query`）：  
   - 目标：`actor` / `object_name` / `pawn_class`  
   - `variables`：点分路径数组  
   - `provocations`：`set_control_rotation` / `add_movement_input` / `jump` / `console_command`  
2. `poll_pie_smoke` 取 `timeseries`。

### E. Live AnimInstance / 回放

- PIE 中：`animation_query` → `sample_pie_anim_instance`。  
- 录制后：`rewind_debugger_query` → `sample_anim_nodes` / `sample_state_machines` / `sample_skeletal_pose` 等。

---

## Camera — 镜头与画面

| 目标 | 做法 |
|------|------|
| 场景 / 材质 / 动画预览图 | `capture_scene_preview`、`capture_anim_frames`、`capture_material_grid`、`capture_with_overlay` |
| PIE 中对准角色拍序列 | `capture_pie_movement_clip` + `view_target_actor` |
| CameraBlueprint 图调试回放 | 启用 **AxonRewindDebugger** + **CameraBlueprint** → `sample_camera_graph_result` / `sample_camera_watches` |

说明：当前没有单独的 `camera_query` 插件；镜头图逻辑仍在项目 Camera 方案中，Axon 负责 **捕获与回放采样**。

---

## Control — 输入与能力

### PIE 运行时注入

| 需求 | Action |
|------|---------|
| Enhanced Input | `pie_inject_input_action` |
| 控制旋转 | `pie_set_control_rotation` |
| 控制台 | `run_console_command` 或 smoke 的 `console_script` |
| 旁观自由相机 | `pie_possess_spectator_free`（若适用） |

可与 timeseries provocations 组合：采样的同时按时间表施加输入。

### 编辑期：GAS / 输入绑定

1. `gas_query` → `scaffold_input_binding_component` / `setup_ability_input_binding` / `bind_ability_to_input`。  
2. `grant_ability_to_pawn` 等（按项目流程）。  
3. 用 PIE smoke 或 `snapshot_gas_state` / `trace_ability_activation` 验证。

### 配置排查

`config_query`：`resolve_setting`、`explain_setting`、`search_config`、`diff_from_default`。写入类 action 仅限开发场景，改前确认环境。

---

## 推荐「最小闭环」

**验证某个 Locomotion 改动是否明显坏掉：**

```text
list_errored_blueprints
  → run_pie_smoke (map + duration + sample_vars + console_script)
  → poll_pie_smoke（直到 complete）
  → 查看 ok / log_patterns / samples
```

**验证输入是否驱动角色：**

```text
start_pie 或 run_pie_smoke
  → pie_inject_input_action / pie_set_control_rotation
  → pie_get_object_properties 或 sample_pie_timeseries
  → stop_pie / stop_pie_smoke
```

---

## 安全与礼仪

- 默认不要用 `on_compile_errors=suppress` 掩盖坏蓝图。  
- 换关前停掉 PIE / session，避免 World Leak。  
- `bulk_fill` 先 `dry_run=true`。  
- `save_packages` 前看 `list_dirty_packages`，避免误存无关资产。  
- 长时间占用 PIE 时告知协作者（编辑器会被 Play 占用）。

---

## 下一步

- 扩展自己的域工具：[SIBLING_PLUGIN_GUIDE.md](SIBLING_PLUGIN_GUIDE.md)  
- 规格与端口：[SPEC_CORE.md](SPEC_CORE.md)  
- 排错：[USER_GUIDE.md](USER_GUIDE.md) §6  
