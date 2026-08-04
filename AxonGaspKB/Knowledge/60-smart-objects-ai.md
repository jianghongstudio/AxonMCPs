> **角色**：Smart Object 与 AI 动画  
> **何时阅读**：查 SmartObject 动画schema或 AI 用 PSD 时  
> **相关资产**：`/Game/Blueprints/SmartObjects/PSS_SmartObject`（asset_index）

# 60 — Smart Objects 与 AI

## Smart Object Pose Search

asset_index（`_raw/asset_index/search_results.json`）包含：

| 资产 | 路径 | 类 |
|------|------|-----|
| PSS_SmartObject | `/Game/Blueprints/SmartObjects/PSS_SmartObject` | PoseSearchSchema |

用于 Smart Object 交互动作的 pose 特征定义（与 locomotion `PSS_Default` 区分）。

## 与玩家 Locomotion 关系

- 玩家主路径：`CHT_PoseSearchDatabases` + State Controller
- Smart Object：独立 Schema，供 AI/交互 Montage 或 SO 插件搜索

## AI 角色

GASP 关卡内 AI 使用相同 Mannequin 骨架 `/Game/Characters/UEFN_Mannequin/Meshes/SK_UEFN_Mannequin`；具体 AI Controller / BT 见 `/Game/Blueprints/` 下 Sandbox 相关 BP（未完整 dump 时以编辑器为准）。

## 调试

| CVar | 用途 |
|------|------|
| `DDCvar.DrawCharacterDebugShapes` | 角色调试形状 |
| `DDCvar.DrawCharacterDebugStates` | 状态文字 |
| `DDCvar.DrawCharacterDebugGraphs` | 图表 |
