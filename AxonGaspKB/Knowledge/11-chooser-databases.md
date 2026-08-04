> **角色**：Chooser 表结构与 PSD 路由  
> **何时阅读**：查某 MovementMode/Gait 组合对应哪个 PSD 子库时  
> **相关资产**：`/Game/Characters/UEFN_Mannequin/Animations/MotionMatchingData/CHT_PoseSearchDatabases*`

# 11 — Chooser 数据库

## 根表 CHT_PoseSearchDatabases

- **路径**：`/Game/Characters/UEFN_Mannequin/Animations/MotionMatchingData/CHT_PoseSearchDatabases`
- **Context**：`SandboxCharacter_CMC_ABP_C`
- **输入列**：`MMDatabaseLOD`（FloatRange）
- **Fallback**：`CHT_PoseSearchDatabases_Dense`

| LOD | MMDatabaseLOD 范围 | 子 Chooser |
|-----|-------------------|------------|
| 0 | [0, 0] | `CHT_PoseSearchDatabases_Dense` |
| 1 | [1, 1] | `CHT_PoseSearchDatabases_Sparse` |
| 2 | [2, 2] | `CHT_PoseSearchDatabases_ExtremeSparse` |


## Dense 表列定义

`CHT_PoseSearchDatabases_Dense` — 4 个 Enum 输入列：

| 列 | 绑定变量 | 类型 |
|----|----------|------|
| 0 | `MovementMode` | Enum |
| 1 | `Stance` | Enum |
| 2 | `MovementState` | Enum |
| 3 | `Gait` | Enum |

## Dense 行矩阵（来自 `_raw/.../CHT_PoseSearchDatabases_Dense.json`）

| 行 | 嵌套表 | MovementMode | Stance | MovementState | Gait |
|----|--------|--------------|--------|---------------|------|
| 0 | Stand Idles | = Grounded (0) | = Stand (0) | = Idle (0) | ≠ Walk (0) |
| 1 | Stand Walks | = Grounded (0) | = Stand (0) | = Moving (1) | = Walk (0) |
| 2 | Stand Runs | = Grounded (0) | = Stand (0) | = Moving (1) | = Run (1) |
| 3 | Stand Sprint | = Grounded (0) | = Stand (0) | = Moving (1) | = Sprint (2) |
| 4 | InAir | = InAir (1) | ≠ Stand (0) | ≠ Idle (0) | ≠ Walk (0) |
| 5 | Crouch Idle | = Grounded (0) | = Crouch (1) | = Idle (0) | ≠ Walk (0) |
| 6 | Crouch Moving | = Grounded (0) | = Crouch (1) | = Moving (1) | ≠ Walk (0) |


### 行 → PSD 嵌套表示例

| 行 | 代表 PSD（Stand Idles 子表示例） |
|----|----------------------------------|
| 0 | `/Game/.../Dense/PSD_Dense_Stand_Idles`, `PSD_Dense_Stand_TurnInPlace`, Stops... |
| 1 | `PSD_Dense_Stand_Walk_Starts`, `PSD_Dense_Stand_Walk_Loops`, `PSD_Dense_Stand_Walk_Pivots`... |
| 2 | `PSD_Dense_Stand_Run_*` |
| 3 | `PSD_Dense_Stand_Sprint_*` |
| 4 | `PSD_Dense_Jumps`, `PSD_Dense_Jumps_Far`, `PSD_Dense_Jumps_FromTraversal` |
| 5 | `PSD_Dense_Crouch_Idles`, `PSD_Dense_Crouch_TurnInPlace` |
| 6 | `PSD_Dense_Crouch_Walk_*` |

## 其他 PSD Chooser

| Chooser | 路径 |
|---------|------|
| Sparse | `/Game/Characters/UEFN_Mannequin/Animations/MotionMatchingData/CHT_PoseSearchDatabases_Sparse` |
| ExtremeSparse | `.../CHT_PoseSearchDatabases_ExtremeSparse` |
| Relaxed | `.../CHT_PoseSearchDatabases_Relaxed` |
| Traversal PSD | `/Game/Characters/UEFN_Mannequin/Animations/MotionMatchingData/Databases/PSD_Traversal` |

## Traversal Montage Chooser

| 资产 | 路径 | Context |
|------|------|---------|
| CMC | `/Game/Characters/UEFN_Mannequin/Animations/Traversal/CHT_TraversalMontages_CMC` | `S_TraversalChooserInputs/Outputs` |
| Mover | `.../CHT_TraversalMontages_Mover` | 同上 |
| 重定向 | `CHT_TraversalAnims_PoseMatch` → 重定向到 CMC 表 |

## Camera Chooser

`/Game/Blueprints/Cameras/CHT_CameraRig` — 输入 `CameraStyle` + `CameraMode`（来自 `S_CharacterPropertiesForCamera`），11 行 → `CameraRigAsset`。
