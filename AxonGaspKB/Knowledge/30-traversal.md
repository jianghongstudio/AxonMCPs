> **角色**：Traversal 翻越系统  
> **何时阅读**：调试 Hurdle/Mantle/Vault 选片与检测条件时  
> **相关资产**：`/Game/Characters/UEFN_Mannequin/Animations/Traversal/CHT_TraversalMontages_CMC`

# 30 — Traversal

## 资产

| 类型 | 路径 |
|------|------|
| Chooser CMC | `/Game/Characters/UEFN_Mannequin/Animations/Traversal/CHT_TraversalMontages_CMC` |
| Chooser Mover | `/Game/Characters/UEFN_Mannequin/Animations/Traversal/CHT_TraversalMontages_Mover` |
| Pose Match 库 | `/Game/Characters/UEFN_Mannequin/Animations/MotionMatchingData/Databases/PSD_Traversal` |
| 输入 Struct | `/Game/Blueprints/Data/S_TraversalChooserInputs` |
| 输出 Struct | `/Game/Blueprints/Data/S_TraversalChooserOutputs` |

## Chooser 列（CMC dump）

| 列 | 字段 | 说明 |
|----|------|------|
| 0 | HasFrontLedge | 前方 ledge |
| 1 | HasBackLedge | 后方 ledge |
| 2 | HasBackFloor | 后方地面 |
| 3 | ObstacleDepth | 障碍深度 cm |
| 4 | BackLedgeHeight | 后方 ledge 高度 cm |
| 5 | Output | `E_TraversalActionType` |

## 行 → 动作类

| 行 | 嵌套表 | 典型条件摘要 |
|----|--------|--------------|
| 0 | Hurdles | 有前后 ledge + 有后地面；深度 0–59；后沿高度 ≥50 |
| 1 | Mantles | 同上；后沿高度 0–10 |
| 2 | Vaults | 有前后 ledge；**无**后地面；深度 0–59 |
| 3 | Mantles | 有前 ledge；深度 ≥59 |

## Hurdle Montage 示例（子表前 5 条）

| # | 资产路径 |
|---|----------|
| 0 | `/Game/Characters/UEFN_Mannequin/Animations/Traversal/Hurdle/AM_M_Neutral_Traversal_Hurdle_1_0_stand_F_V2_Lfoot` |
| 1 | `.../AM_M_Neutral_Traversal_Hurdle_1_0_walk_F_Lfoot` |
| 2 | `.../AM_M_Neutral_Traversal_Hurdle_1_0_walk_F_Rfoot` |
| 3 | `.../AM_M_Neutral_Traversal_Hurdle_1_0_run_F_Lfoot` |
| 4 | `.../AM_M_Neutral_Traversal_Hurdle_1_0_run_F_Rfoot` |

Catch 动画：`.../Traversal/Catch/Hurdle/AM_M_Neutral_Traversal_Catch_Hurdle_*`

## 调试 CVar

| CVar | 默认 | 用途 |
|------|------|------|
| `DDCvar.Traversal.DrawDebugLevel` | 0 | 绘制检测 |
| `DDCvar.Traversal.DrawDebugDuration` | 1.0 | Debug 持续时间 |

## 与 Locomotion 衔接

- InAir PSD 含 `PSD_Dense_Jumps_FromTraversal`
- Run/Walk 含 `PSD_Dense_*_FromTraversal` 过渡库
