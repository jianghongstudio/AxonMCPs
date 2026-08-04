> **角色**：术语与枚举对照  
> **何时阅读**：阅读过渡表、Chooser 列、调试状态时查阅  
> **相关资产**：`/Game/Blueprints/Data/E_*` 枚举资产

# 02 — 术语表

## Locomotion 核心枚举（来自 Chooser 列绑定与 ABP 变量）

### E_MovementMode (`/Game/Blueprints/Data/E_MovementMode`)

| 值 | 名称 | 证据 |
|----|------|------|
| 0 | Grounded | `CHT_PoseSearchDatabases_Dense` 多数行；`-> Grounded` 过渡 |
| 1 | InAir | Dense row 4；`-> In Air` 过渡 |
| 2 | Slide | Mover State Controller `-> Slide`；Dense 未单独列 Slide 行 |

### E_Stance

| 值 | 名称 | 证据 |
|----|------|------|
| 0 | Stand | Dense rows 0–4 |
| 1 | Crouch | Dense rows 5–6 |

### E_MovementState

| 值 | 名称 | 证据 |
|----|------|------|
| 0 | Idle | Dense row 0；Mover Conduit 用 `MovementState` 判 Idle |
| 1 | Moving | Dense rows 1–3, 6；Mover `Idle -> Locomotion` 过渡 |

### E_Gait

| 值 | 名称 | 证据 |
|----|------|------|
| 0 | Walk | Dense row 1 |
| 1 | Run | Dense row 2 |
| 2 | Sprint | Dense row 3 |

### E_MovementDirection

用于 pivot / 方向重选；ABP 变量 `MovementDirection`、`MovementDirection_LastFrame`（Mover 带下划线后缀）。

## State Controller 状态名

| 状态 | 类型 | 含义 |
|------|------|------|
| `Idle Loop` | Loop | 站立 idle 循环 |
| `Locomotion Loop` | Loop | 行走/奔跑循环 |
| `In Air Loop` | Loop | 空中循环 |
| `Slide Loop` | Loop | **仅 Mover** 滑铲循环 |
| `Transition to *` | Transition | 搜索并播放过渡动画 |
| `Idle Break` | Sub | Idle 内插入 break 动画 |
| `Conduit` | Conduit | 落地后分流 Idle/Locomotion |
| `Re-Enter` | 伪状态 | Loop 内重选动画入口 |

## 关键 ABP 函数

| 函数 | 路径 | 说明 |
|------|------|------|
| `IsMoving()` | CMC ABP | 当前/未来速度或加速度非零（见 `_raw/abp/cmc/IsMoving.json` 注释） |
| `IsPivoting()` | CMC ABP | 未来轨迹与当前方向偏差大；MM 与 Experimental SM 两套条件 |
| `ShouldTurnInPlace()` | CMC ABP | 根骨与胶囊朝向差 >50° 且 aiming；或刚停止（Stick Flick） |

## Chooser 术语

| 术语 | 含义 |
|------|------|
| `MMDatabaseLOD` | FloatRange 输入列，0=Dense, 1=Sparse, 2=ExtremeSparse |
| `EvaluateChooser` | 嵌套子 Chooser |
| `NestedChooser` | Dense 内 Stand Idles / Stand Walks 等分组 |
| `AssetChooser` | 最终 PSD 或 Montage 资产 |

## Traversal 术语

| 字段 | 来源 |
|------|------|
| `HasFrontLedge` / `HasBackLedge` / `HasBackFloor` | `S_TraversalChooserInputs` 布尔列 |
| `ObstacleDepth` / `BackLedgeHeight` | FloatRange 列（cm） |
| Hurdle / Mantle / Vault | `CHT_TraversalMontages_CMC` 嵌套表 |

## 缩写

| 缩写 | 展开 |
|------|------|
| GASP | Game Animation Sample |
| MM | Motion Matching |
| PSD | PoseSearchDatabase |
| PSS | PoseSearchSchema |
| CMC | Character Movement Component |
| CHT | Chooser Table |
| CVar / DDCvar | DefaultEngine.ini 中 `DDCvar.*` 控制台变量 |
