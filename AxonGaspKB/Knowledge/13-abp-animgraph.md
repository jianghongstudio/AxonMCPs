> **角色**：AnimBP 动画图与更新函数  
> **何时阅读**：理解 AnimGraph 节点链、ThreadSafe 更新、BlendStack 时  
> **相关资产**：`/Game/Blueprints/SandboxCharacter_CMC_ABP`, `/Game/Blueprints/SandboxCharacter_Mover_ABP`

# 13 — AnimBP 动画图

## 目标资产

| AnimBP | 路径 |
|--------|------|
| CMC | `/Game/Blueprints/SandboxCharacter_CMC_ABP` |
| Mover | `/Game/Blueprints/SandboxCharacter_Mover_ABP` |

## Event Graph / 更新管线

### Update_Logic（CMC，`_raw/abp/cmc/Update_Logic.json`）

```
Update_Trajectory → Update_EssentialValues → Update_States
  → [Branch: UseExperimentalStateMachine] → Update_MovementDirection → Update_TargetRotation
```

### Update_CVarDrivenVariables

缓存 CVar 到 AnimBP 变量（非 ThreadSafe 读 CVar）：

| 变量 | CVar |
|------|------|
| `OffsetRootBoneEnabled` | `DDCVar.AttributeBasedRootMotion.Enable` |
| `MMDatabaseLOD` | `DDCvar.MMDatabaseLOD` |
| `UseExperimentalStateMachine` | `DDCVar.ExperimentalStateMachine.Enable` |
| `DebugExperimentalStateMachine` | `DDCVar.ExperimentalStateMachine.Debug` |
| `OffsetRootTranslationRadius` | `DDCvar.OffsetRootBone.TranslationRadius` |
| `UseThreadSafeUpdateAnimation` | `DDCVar.ThreadSafeAnimationUpdate.Enable` |
| `LocomotionSetup` | `DDCvar.LocomotionSetupCMC` / Mover 变体 |

### 辅助函数

| 函数 | 说明 |
|------|------|
| `IsMoving()` | 速度/未来速度/加速度判断移动意图 |
| `IsPivoting()` | 轨迹方向突变；分 MM 与 SM 复合条件 |
| `ShouldTurnInPlace()` | 瞄准态根骨偏差 >50° 或刚停止 |

## AnimGraph 主要节点（概念）

| 节点/子系统 | 作用 |
|-------------|------|
| **State Controller** | 逻辑 SM，见 `14-state-controller.md` |
| **BlendStack** | 多层动画混合；承载 MM 搜索结果 |
| **Evaluate Chooser** | 绑定 `CHT_PoseSearchDatabases*` |
| **Motion Matching** | Pose Search 节点 |
| **Offset Root Bone** | 根骨平移/旋转偏移（CVar 控制） |
| **Foot Placement** | 足部 IK（`DDCvar.FootPlacementMode`） |
| **Trajectory** | 生成 `Trj_*` 变量（Mover 更重） |

## BlendStack 相关变量

| 变量 | 过渡表用途 |
|------|------------|
| `BlendStackInputs` | Break struct；检查动画标签 |
| `NoValidAnim` | 无有效搜索结果显示 |
| `NotifyTransition_Re-Transition` | Early transition notify |
| `NotifyTransition_ToLoop` | 提前进入 Loop |

## Mover 特有 AnimGraph 变量

| 变量 | 用途 |
|------|------|
| `Trj_FutureVelocity` / `Trj_FutureFacing` | 轨迹预测 |
| `Trj_IsCircling` / `Trj_CirclingTime` | 绕圈检测 |
| `FutureFacingDelta` | Turn in place / 旋转 break |
| `MovementMode_Recent` | 近期模式（Pivot 抑制） |
