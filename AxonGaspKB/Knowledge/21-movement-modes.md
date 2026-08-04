> **角色**：Mover MovementMode 蓝图  
> **何时阅读**：扩展 Mover 模式、理解 Slide/Walk/Fall 切换时  
> **相关资产**：`/Game/Blueprints/MovementModes/BP_MovementMode_*`

# 21 — Movement Modes（Mover）

Mover 插件通过 **Movement Mode** 蓝图扩展 locomotion。GASP 包含以下模式资产（`_raw/blueprints/` dump）：

## 模式清单

| 蓝图 | 路径 | 父类 | 图 |
|------|------|------|-----|
| Walking | `/Game/Blueprints/MovementModes/BP_MovementMode_Walking` | `SmoothWalkingMode` | EventGraph, `GenerateWalkMove` |
| Falling | `/Game/Blueprints/MovementModes/BP_MovementMode_Falling` | Mover Falling | EventGraph |
| Slide | `/Game/Blueprints/MovementModes/BP_MovementMode_Slide` | — | EventGraph |

## 过渡蓝图

| 资产 | 路径 |
|------|------|
| To Slide | `/Game/Blueprints/MovementModes/BP_MovementTransition_ToSlide` |
| From Slide | `/Game/Blueprints/MovementModes/BP_MovementTransition_FromSlide` |

## 与 AnimBP 联动

| MovementMode 枚举 | AnimBP 状态 |
|-------------------|-------------|
| Grounded (0) | Conduit → Idle/Locomotion |
| InAir (1) | `Transition to In Air` → `In Air Loop` |
| Slide (2) | **Mover** `Transition to Slide` → `Slide Loop` |

Walking 模式变量数：15；Slide/Falling 见对应 `__info.json` dump。

## 设计要点

- MovementMode 由 Mover 组件写入，AnimBP `Update_States` 同步到 `MovementMode` 供 Chooser 与 State Controller 使用。
- Slide 过渡在 State Controller 有独立 Transition/Loop/Re-Enter（`MovementDirection` 变化触发重选）。
