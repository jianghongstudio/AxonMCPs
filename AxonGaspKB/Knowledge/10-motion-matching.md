> **角色**：Motion Matching / Pose Search 专题  
> **何时阅读**：理解 PSD 结构、Schema 通道、MM 搜索特征时  
> **相关资产**：`/Game/Characters/UEFN_Mannequin/Animations/MotionMatchingData/`, `/Game/Characters/UEFN_Mannequin/Animations/MotionMatchingData/Schemas/PSS_Default`

# 10 — Motion Matching

GASP 使用 UE **Pose Search** 插件做 Motion Matching。AnimBP 通过 **Evaluate Chooser** 选出 `PoseSearchDatabase`，再在 BlendStack 内搜索最佳 pose/segment。

## 核心资产

| 类型 | 路径 |
|------|------|
| 根 Chooser | `/Game/Characters/UEFN_Mannequin/Animations/MotionMatchingData/CHT_PoseSearchDatabases` |
| 默认 Schema | `/Game/Characters/UEFN_Mannequin/Animations/MotionMatchingData/Schemas/PSS_Default` |
| Idle Schema | `.../Schemas/PSS_Idle` |
| Jump Schema | `.../Schemas/PSS_Jump` |
| Stop Schema | `.../Schemas/PSS_Stop` |
| Traversal Schema | `.../Schemas/PSS_Traversal` |
| Relaxed Schema | `.../Schemas/PSS_Relaxed_Loops` |
| 骨架 | `/Game/Characters/UEFN_Mannequin/Meshes/SK_UEFN_Mannequin` |

## PSS_Default 结构（dump 证据）

| 属性 | 值 |
|------|-----|
| Sample Rate | 30 Hz |
| Schema Cardinality | 30 |
| 通道 0 | `PoseSearchFeatureChannel_Trajectory`，cardinality=19，5 个时间采样 |
| 通道 1 | `PoseSearchFeatureChannel_Group`，cardinality=11 |

轨迹采样点（部分）：

| Offset (s) | Weight |
|------------|--------|
| -0.05 | 0.3 |
| 0 | 1.0 |
| 0.35 | 1.0 |
| 0.7 | 1.0 |
| 1.0 | 1.5 |

## LOD 与 Chooser 层级

由 `DDCvar.MMDatabaseLOD`（0/1/2）驱动 `CHT_PoseSearchDatabases` 的 FloatRange 列，详见 `11-chooser-databases.md`。

## Experimental State Machine 数据（可选路径）

当 `DDCVar.ExperimentalStateMachine.Enable=true` 时，可使用：

| PSD | 路径 |
|-----|------|
| CMC Idles | `/Game/Characters/UEFN_Mannequin/Animations/ExperimentalStateMachineData/PSD_SM_CMC_Idles` |
| CMC Loops | `.../PSD_SM_CMC_Loops` |
| CMC Transitions | `.../PSD_SM_CMC_Transitions` |
| Mover Loops | `.../PSD_SM_Mover_Loops` |

## Mover Relaxed 集

Mover AnimBP 引用 `/Game/Characters/UEFN_Mannequin/Animations/MotionMatchingData/CHT_PoseSearchDatabases_Relaxed`（见 asset_index）。

## 与 State Controller 关系

- **Transition 状态**：对 Transition 类 PSD 做搜索，可与 Loop PSD 竞争（见 `14-state-controller.md` 注释）。
- **Re-Enter**：属性变化触发重新进入 Transition，Chooser 条件（如 `IsPivoting`）同步参与选片。
