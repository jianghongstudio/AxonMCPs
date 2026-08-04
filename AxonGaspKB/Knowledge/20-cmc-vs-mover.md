> **角色**：CMC 与 Mover 双路径对比  
> **何时阅读**：选择/迁移 locomotion 方案或对比行为差异时  
> **相关资产**：`/Game/Blueprints/SandboxCharacter_CMC`, `/Game/Blueprints/SandboxCharacter_Mover`

# 20 — CMC vs Mover

GASP 同时展示 **Character Movement Component** 与 **Mover Plugin** 两套 locomotion 驱动。

## Pawn 与 AnimBP

| | CMC | Mover |
|---|-----|-------|
| 角色 BP | `/Game/Blueprints/SandboxCharacter_CMC` | `/Game/Blueprints/SandboxCharacter_Mover` |
| AnimBP | `/Game/Blueprints/SandboxCharacter_CMC_ABP` | `/Game/Blueprints/SandboxCharacter_Mover_ABP` |
| PSD Chooser | `CHT_PoseSearchDatabases` | `CHT_PoseSearchDatabases_Relaxed`（asset_index） |
| Traversal | `CHT_TraversalMontages_CMC` | `CHT_TraversalMontages_Mover` |

## 移动检测

| 场景 | CMC | Mover |
|------|-----|-------|
| 是否移动 | `IsMoving()` 函数（速度+轨迹） | `MovementState` 枚举 |
| Idle→Locomotion | `IsMoving()` | `MovementState == Moving` |
| Conduit | `IsMoving()` | `MovementState` |

## State Controller

| | CMC | Mover |
|---|-----|-------|
| 状态数 | 7 | 9（+Slide） |
| 过渡数 | 22 | 29 |
| 旋转参考 | `TargetRotation` | `Trj_FutureFacing` |
| 特殊 | — | Slide、Circling、InAir 方向 Re-Enter |

## CVar 默认

| CVar | 默认 | 含义 |
|------|------|------|
| `DDCvar.LocomotionSetupCMC` | 0 | CMC 配置索引 |
| `DDCvar.LocomotionSetupMover` | 1 | Mover 配置索引 |

## 何时选哪条路径

| 需求 | 建议 |
|------|------|
| 传统第三人称、简单复制 | CMC |
| 预测轨迹、Slide、新移动框架 | Mover |
| 调试 MM 密度 | 共用 `MMDatabaseLOD`，Chooser 表不同 |
