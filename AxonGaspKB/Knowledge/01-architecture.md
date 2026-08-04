> **角色**：GASP 动画系统总览  
> **何时阅读**：需要理解 CMC/Mover、MM、Chooser、State Controller 如何协作时  
> **相关资产**：`/Game/Blueprints/SandboxCharacter_CMC_ABP`, `/Game/Blueprints/SandboxCharacter_Mover_ABP`, `/Game/Characters/UEFN_Mannequin/Animations/MotionMatchingData/CHT_PoseSearchDatabases`

# 01 — 架构总览

## 分层模型

```
┌─────────────────────────────────────────────────────────┐
│  Pawn BP: SandboxCharacter_CMC / SandboxCharacter_Mover │
│  Movement: CharacterMovementComponent / Mover Plugin    │
└──────────────────────────┬──────────────────────────────┘
                           │ S_CharacterPropertiesFor*
                           ▼
┌─────────────────────────────────────────────────────────┐
│  AnimBP: SandboxCharacter_*_ABP                         │
│  Update_Logic → Update_States → State Controller (SM)   │
│  AnimGraph: BlendStack + Motion Matching / Traversal      │
└──────────────────────────┬──────────────────────────────┘
                           │ Evaluate Chooser
                           ▼
┌─────────────────────────────────────────────────────────┐
│  CHT_PoseSearchDatabases* → PSD_* PoseSearchDatabase    │
│  CHT_TraversalMontages_*  → AnimMontage                 │
│  CHT_CameraRig            → CameraRigAsset              │
└─────────────────────────────────────────────────────────┘
```

## 核心 AnimBP

| 资产 | 路径 | 用途 |
|------|------|------|
| CMC AnimBP | `/Game/Blueprints/SandboxCharacter_CMC_ABP` | 传统 CMC locomotion + MM |
| Mover AnimBP | `/Game/Blueprints/SandboxCharacter_Mover_ABP` | Mover 轨迹驱动 + Slide 状态 |

## 每帧更新链（CMC，来自 `_raw/abp/cmc/Update_Logic.json`）

| 顺序 | 函数 | 作用 |
|------|------|------|
| 1 | `Update_Trajectory` | 生成/更新轨迹采样 |
| 2 | `Update_EssentialValues` | 速度、加速度等基础量 |
| 3 | `Update_States` | 写入 `MovementMode`/`MovementState`/`Gait`/`Stance` 及 `_LastFrame` |
| 4 | （分支） | `UseExperimentalStateMachine` 为真时跳过部分逻辑 |
| 5 | `Update_MovementDirection` | 移动方向枚举 |
| 6 | `Update_TargetRotation` | 目标朝向 |

`Update_CVarDrivenVariables` 在游戏线程缓存 CVar（见 `80-ddcvars.md`），供 ThreadSafe 函数读取。

## State Controller 设计理念

- **纯逻辑状态机**：状态节点本身不绑动画；动画由 **BlendStack + Chooser + Pose Search** 驱动。
- **Transition 状态**：进入 `Transition to Locomotion/Idle/In Air/Slide` 时搜索过渡动画；满足「动画播完 / 无有效动画 / 提前 Notify」后进入 Loop。
- **Re-Enter 状态**：在 Loop 中监听 `Stance`/`Gait`/`MovementDirection` 变化、pivot、转身、旋转 break 等，触发重选动画。

## Chooser 在架构中的位置

| Chooser | 路径 | 输出 |
|---------|------|------|
| PSD 根 | `/Game/Characters/UEFN_Mannequin/Animations/MotionMatchingData/CHT_PoseSearchDatabases` | `PoseSearchDatabase` |
| PSD Dense | `.../CHT_PoseSearchDatabases_Dense` | 按 MovementMode/Stance/MovementState/Gait 分子表 |
| Traversal CMC | `.../Traversal/CHT_TraversalMontages_CMC` | `AnimMontage` |
| Traversal Mover | `.../Traversal/CHT_TraversalMontages_Mover` | `AnimMontage` |
| Camera | `/Game/Blueprints/Cameras/CHT_CameraRig` | `CameraRigAsset` |

## 角色属性 Struct（跨系统总线）

| Struct | 路径 | 消费方 |
|--------|------|--------|
| `S_CharacterPropertiesForAnimation` | `/Game/Blueprints/Data/S_CharacterPropertiesForAnimation` | AnimBP、PSD Chooser |
| `S_CharacterPropertiesForTraversal` | `/Game/Blueprints/Data/S_CharacterPropertiesForTraversal` | Traversal 检测 |
| `S_CharacterPropertiesForCamera` | `/Game/Blueprints/Data/S_CharacterPropertiesForCamera` | `CHT_CameraRig` |

## 实验性功能开关

| CVar | 默认 | 影响 |
|------|------|------|
| `DDCVar.ExperimentalStateMachine.Enable` | false | 切换 Experimental SM 数据路径 |
| `DDCVar.ThreadSafeAnimationUpdate.Enable` | true | 线程安全动画更新 |
| `DDCVar.NewGameplayCameraSystem.Enable` | true | 新 Gameplay Camera |
| `DDCvar.MMDatabaseLOD` | 0 | Dense/Sparse/ExtremeSparse PSD 层级 |
