# -*- coding: utf-8 -*-
"""Generate GASP Knowledge markdown corpus from _raw JSON dumps."""
import json
import re
from pathlib import Path

RAW = Path(__file__).resolve().parent / "_raw"
OUT = Path(__file__).resolve().parent
written = []


def load(rel):
    with open(RAW / rel, encoding="utf-8") as f:
        return json.load(f)


def save(name, content):
    path = OUT / name
    path.write_text(content.rstrip() + "\n", encoding="utf-8")
    lines = content.count("\n") + 1
    written.append((name, lines))


def role_block(role, when, assets):
    return (
        f"> **角色**：{role}  \n"
        f"> **何时阅读**：{when}  \n"
        f"> **相关资产**：{assets}\n"
    )


def extract_keys(rule_nodes):
    keys = []
    comments = []
    for n in rule_nodes:
        cls = n.get("class", "")
        title = n.get("title", "")
        if cls == "EdGraphNode_Comment":
            c = n.get("title", "") if "title" in n and len(n.get("title", "")) > 20 else ""
            if not c:
                c = n.get("comment", title)
            if c and c not in ("Comment",):
                comments.append(c.replace("\r\n", " ").replace("\n", " "))
        elif cls in ("K2Node_VariableGet", "K2Node_CallFunction", "K2Node_AnimGetter"):
            t = title.replace("Get ", "").replace("Target is Anim Instance", "").strip()
            if t and t not in ("Result",):
                keys.append(t)
        elif cls == "K2Node_BreakStruct":
            keys.append(title)
    return keys, comments


def distill_condition(rule_nodes):
    keys, comments = extract_keys(rule_nodes)
    parts = []
    fn_map = {
        "Is Moving": "角色正在移动（`IsMoving()`：当前/未来速度或加速度非零）",
        "NOT Boolean": "取反",
        "Is Animation Almost Complete": "BlendStack 中当前动画接近播放完毕",
        "Is Pivoting": "正在 pivot（`IsPivoting()`）",
        "Should Turn in Place": "应原地转身（`ShouldTurnInPlace()`）",
        "Delta (Rotator)": "Rotator 角度差",
        "Absolute (Float)": "取绝对值",
        "Get Curve Value": "读取动画曲线值",
    }
    var_seen = set()
    for k in keys:
        if k.startswith("Get "):
            v = k[4:]
            if v not in var_seen:
                var_seen.add(v)
                parts.append(f"`{v}`")
        elif k in fn_map:
            parts.append(fn_map[k])
        elif "Contains Item" in k:
            parts.append("BlendStack 动画标签匹配")
        elif k == "Break S Blend Stack Inputs":
            parts.append("解析 `BlendStackInputs`")
        elif "Equal (Enum)" in k or "Not Equal (Enum)" in k:
            continue
        elif "Current State Time (State Controller)" in k:
            parts.append("`CurrentStateTime` 阈值判断")
        elif "Select" in k:
            parts.append("条件选择")
        elif "float > float" in k or "float < float" in k or "float >= float" in k or "float - float" in k:
            continue
        elif "AND Boolean" in k or "OR Boolean" in k:
            continue
        else:
            parts.append(f"`{k}`")
    summary = "；".join(parts[:8]) if parts else "（见注释）"
    if comments:
        summary += f"。{comments[0][:200]}"
    return summary


def transition_table(transitions, abp_path):
    rows = []
    for i, t in enumerate(transitions, 1):
        keys, comments = extract_keys(t.get("rule_nodes", []))
        cond = distill_condition(t.get("rule_nodes", []))
        key_str = ", ".join(f"`{k.replace('Get ', '')}`" for k in keys[:12])
        rows.append(
            f"| {i} | `{t['from']}` | `{t['to']}` | {t.get('from_type','')}→{t.get('to_type','')} "
            f"| {t.get('cross_fade_duration', 0):.2f}s | {cond} | {key_str} |"
        )
    header = (
        f"## 完整过渡表\n\n"
        f"来源：`_raw/abp/.../transitions_StateController.json`，AnimBP `{abp_path}`\n\n"
        f"| # | 源 | 目标 | 类型 | CrossFade | 条件摘要 | 关键变量/函数 |\n"
        f"|---|-----|------|------|-----------|----------|---------------|\n"
    )
    return header + "\n".join(rows)


def enum_cell(val, comp, enum_name, labels):
    if comp == 2:
        op = "≠"
    elif comp == 0:
        op = "="
    else:
        op = f"cmp{comp}"
    if isinstance(val, int) and val < len(labels):
        return f"{op} {labels[val]} ({val})"
    return f"{op} {val}"


# --- Generate files ---
MM = ["Grounded", "InAir", "Slide"]
STANCE = ["Stand", "Crouch"]
MS = ["Idle", "Moving"]
GAIT = ["Walk", "Run", "Sprint"]

# 00-routing
save("00-routing.md", role_block(
    "知识库导航索引",
    "首次打开本语料库时；不确定该读哪篇文档时",
    "全部 `/Game/...` 资产（见各专题文档）"
) + """
# 00 — 路由索引

Epic **Game Animation Sample (GASP)** 离线知识库。范围仅限官方 Sample 内容；不含第三方 AnimStateMachine / AnimTable 等扩展插件。

## 阅读路径

| 目标 | 推荐顺序 |
|------|----------|
| 快速了解整体架构 | `01-architecture` → `02-glossary` → `20-cmc-vs-mover` |
| 调试 locomotion 状态/过渡 | `14-state-controller` → `12-character-properties` → `11-chooser-databases` |
| Motion Matching 选库 | `10-motion-matching` → `11-chooser-databases` → `80-ddcvars`（`MMDatabaseLOD`） |
| Traversal 翻越/攀越 | `30-traversal` → `11-chooser-databases`（Traversal Chooser） |
| 相机切换 | `50-gameplay-camera` |
| Mover 特有（Slide/Trajectory） | `20-cmc-vs-mover` → `21-movement-modes` → `14-state-controller`（Mover 节） |
| 资产定位 | `90-asset-map` |

## 文档清单

| 编号 | 文件 | 主题 |
|------|------|------|
| 00 | `00-routing.md` | 本索引 |
| 01 | `01-architecture.md` | 系统分层与数据流 |
| 02 | `02-glossary.md` | 术语与枚举 |
| 10 | `10-motion-matching.md` | Pose Search / MM |
| 11 | `11-chooser-databases.md` | Chooser 表与 PSD 路由 |
| 12 | `12-character-properties.md` | 角色属性 Struct |
| 13 | `13-abp-animgraph.md` | AnimBP 更新图 |
| 14 | `14-state-controller.md` | State Controller 全过渡表 |
| 20 | `20-cmc-vs-mover.md` | CMC vs Mover 差异 |
| 21 | `21-movement-modes.md` | Mover MovementMode BP |
| 30 | `30-traversal.md` | Traversal 检测与 Montage |
| 40 | `40-warping-footplacement.md` | Root Motion / Foot IK |
| 50 | `50-gameplay-camera.md` | Gameplay Camera |
| 60 | `60-smart-objects-ai.md` | Smart Object / AI |
| 70 | `70-foley-retarget.md` | Foley 与 Retarget |
| 80 | `80-ddcvars.md` | DefaultEngine DDCvar |
| 90 | `90-asset-map.md` | 资产路径索引 |

## 证据来源

所有结论应回溯 `Knowledge/_raw/` 下 JSON dump（**只读，勿改**）。`_manifest.json` 列出 89 个 dump 文件。
""")

# 01-architecture
save("01-architecture.md", role_block(
    "GASP 动画系统总览",
    "需要理解 CMC/Mover、MM、Chooser、State Controller 如何协作时",
    "`/Game/Blueprints/SandboxCharacter_CMC_ABP`, `/Game/Blueprints/SandboxCharacter_Mover_ABP`, `/Game/Characters/UEFN_Mannequin/Animations/MotionMatchingData/CHT_PoseSearchDatabases`"
) + """
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
""")

# 02-glossary
save("02-glossary.md", role_block(
    "术语与枚举对照",
    "阅读过渡表、Chooser 列、调试状态时查阅",
    "`/Game/Blueprints/Data/E_*` 枚举资产"
) + """
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
""")

# Continue in next write - 14-state-controller and others via script logic
cmc_trans = load("abp/cmc/transitions_StateController.json")["transitions"]
mover_trans = load("abp/mover/transitions_StateController.json")["transitions"]
cmc_sm = load("abp/cmc/state_machines.json")
mover_sm = load("abp/mover/state_machines.json")
cmc_names = {x["name"] for x in cmc_sm["state_machines"][0]["states"]}
mover_names = {x["name"] for x in mover_sm["state_machines"][0]["states"]}

save("14-state-controller.md", role_block(
    "State Controller 状态机权威参考",
    "调试 locomotion 过渡、Re-Enter、pivot、Slide（Mover）逻辑时**必读**",
    "`/Game/Blueprints/SandboxCharacter_CMC_ABP`, `/Game/Blueprints/SandboxCharacter_Mover_ABP`"
) + f"""
# 14 — State Controller

AnimGraph 内 **State Controller** 状态机为纯逻辑控制器，驱动 BlendStack 选片，不直接持有动画资产。

## 状态清单对比

| 状态 | CMC ({len(cmc_sm['state_machines'][0]['states'])}) | Mover ({len(mover_sm['state_machines'][0]['states'])}) |
|------|-----|-------|
""" + "\n".join(
    f"| `{name}` | {'✓' if name in cmc_names else '—'} | {'✓' if name in mover_names else '—'} |"
    for name in sorted(cmc_names | mover_names)
) + f"""

- **入口状态**：两者均为 `Transition to Idle`（`_raw/.../state_machines.json`）
- **CMC 过渡数**：{len(cmc_trans)}
- **Mover 过渡数**：{len(mover_trans)}（含 Slide 与 Mover 特有 Re-Enter 规则）

## CMC — `/Game/Blueprints/SandboxCharacter_CMC_ABP`

{transition_table(cmc_trans, '/Game/Blueprints/SandboxCharacter_CMC_ABP')}

## Mover — `/Game/Blueprints/SandboxCharacter_Mover_ABP`

{transition_table(mover_trans, '/Game/Blueprints/SandboxCharacter_Mover_ABP')}

## CMC vs Mover 过渡差异摘要

| 主题 | CMC | Mover |
|------|-----|-------|
| Idle↔Locomotion 入口 | `IsMoving()` / `NOT IsMoving()` | `MovementState` 枚举相等 |
| Conduit 分流 | `IsMoving()` | `MovementState` |
| Pivot Re-Enter | `IsPivoting()` + BlendStack 标签 | 同上 + `Trj_IsCircling`/`MovementMode_Recent` |
| 旋转 Break | `TargetRotation` vs `TargetRotationOnTransitionStart` | `Trj_FutureFacing` vs `FutureFacingOnTransitionStart` |
| Turn in Place | `ShouldTurnInPlace()` | 额外 `FutureFacingDelta`、曲线值 |
| Slide | 无 | `-> Slide`、`Transition to Slide`、`Slide Loop` |
| 绕圈 | 无 | `Trj_CirclingTime`、`Trj_IsCircling` 相关过渡 |

## 共用机制说明

### 自动播完规则（Transition → Loop）

> This acts simliar to the "Automatic Rule" condition in typical state machines...

当 BlendStack 中动画 **almost complete** 且 `CurrentStateTime` 超阈值时进入 Loop。

### 无动画/直接 Loop

> If no animations were found when entering into the transition state...

`NoValidAnim` 为真，或过渡搜索直接命中 Loop 类动画时跳过 Transition。

### Early Transition Notify

`NotifyTransition_Re-Transition` / `NotifyTransition_ToLoop` 由 `/Game/Blueprints/BP_NotifyState_EarlyTransition` 设置。
""")

# 10-motion-matching
dense = load("choosers/Characters__UEFN_Mannequin__Animations__MotionMatchingData__CHT_PoseSearchDatabases_Dense.json")["result"]
pss = load("pose_search/schemas/Characters__UEFN_Mannequin__Animations__MotionMatchingData__Schemas__PSS_Default.json")["result"]
save("10-motion-matching.md", role_block(
    "Motion Matching / Pose Search 专题",
    "理解 PSD 结构、Schema 通道、MM 搜索特征时",
    "`/Game/Characters/UEFN_Mannequin/Animations/MotionMatchingData/`, `/Game/Characters/UEFN_Mannequin/Animations/MotionMatchingData/Schemas/PSS_Default`"
) + f"""
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
| Sample Rate | {pss['sample_rate']} Hz |
| Schema Cardinality | {pss['schema_cardinality']} |
| 通道 0 | `PoseSearchFeatureChannel_Trajectory`，cardinality={pss['channels'][0]['cardinality']}，{pss['channels'][0]['sample_count']} 个时间采样 |
| 通道 1 | `PoseSearchFeatureChannel_Group`，cardinality={pss['channels'][1]['cardinality']} |

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
""")

# 11-chooser-databases
root_cht = load("choosers/Characters__UEFN_Mannequin__Animations__MotionMatchingData__CHT_PoseSearchDatabases.json")["result"]
dense_cols = dense["columns"]
dense_rows = dense["tree"]["rows"]
row_labels = ["Stand Idles", "Stand Walks", "Stand Runs", "Stand Sprint", "InAir", "Crouch Idle", "Crouch Moving"]

def fmt_col(col, row_idx):
    cell = col["cells"][row_idx]
    comp = cell.get("comparison", 0)
    v = cell.get("value", cell.get("min", ""))
    name = col["input_binding"]["display"]
    if name == "MovementMode":
        return enum_cell(v, comp, "MM", MM)
    if name == "Stance":
        return enum_cell(v, comp, "St", STANCE)
    if name == "MovementState":
        return enum_cell(v, comp, "MS", MS)
    if name == "Gait":
        return enum_cell(v, comp, "G", GAIT)
    return str(v)

dense_table = "| 行 | 嵌套表 | MovementMode | Stance | MovementState | Gait |\n|----|--------|--------------|--------|---------------|------|\n"
for i, row in enumerate(dense_rows):
    nested = row["asset"].split(":")[-1] if ":" in row["asset"] else row_labels[i]
    dense_table += f"| {i} | {nested} | {fmt_col(dense_cols[0], i)} | {fmt_col(dense_cols[1], i)} | {fmt_col(dense_cols[2], i)} | {fmt_col(dense_cols[3], i)} |\n"

lod_table = "| LOD | MMDatabaseLOD 范围 | 子 Chooser |\n|-----|-------------------|------------|\n"
for i, row in enumerate(root_cht["columns"][0]["cells"]):
    r = root_cht["tree"]["rows"][i]
    ap = r["asset"].split(".")[0].split("/")[-1]
    lod_table += f"| {i} | [{row['min']}, {row['max']}] | `{ap}` |\n"

save("11-chooser-databases.md", role_block(
    "Chooser 表结构与 PSD 路由",
    "查某 MovementMode/Gait 组合对应哪个 PSD 子库时",
    "`/Game/Characters/UEFN_Mannequin/Animations/MotionMatchingData/CHT_PoseSearchDatabases*`"
) + f"""
# 11 — Chooser 数据库

## 根表 CHT_PoseSearchDatabases

- **路径**：`/Game/Characters/UEFN_Mannequin/Animations/MotionMatchingData/CHT_PoseSearchDatabases`
- **Context**：`SandboxCharacter_CMC_ABP_C`
- **输入列**：`MMDatabaseLOD`（FloatRange）
- **Fallback**：`CHT_PoseSearchDatabases_Dense`

{lod_table}

## Dense 表列定义

`CHT_PoseSearchDatabases_Dense` — 4 个 Enum 输入列：

| 列 | 绑定变量 | 类型 |
|----|----------|------|
| 0 | `MovementMode` | Enum |
| 1 | `Stance` | Enum |
| 2 | `MovementState` | Enum |
| 3 | `Gait` | Enum |

## Dense 行矩阵（来自 `_raw/.../CHT_PoseSearchDatabases_Dense.json`）

{dense_table}

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
""")

# 12-character-properties
anim_refs = load("structs/Blueprints__Data__S_CharacterPropertiesForAnimation__cdo.json")["result"]["properties"]
anim_enums = next(p["value"] for p in anim_refs if p["name"] == "ScriptAndPropertyObjectReferences")

save("12-character-properties.md", role_block(
    "角色属性 Struct 与 ABP 变量",
    "查 MovementMode/Gait/Stance 来源与 Chooser 输入绑定时",
    "`/Game/Blueprints/Data/S_CharacterPropertiesFor*`"
) + f"""
# 12 — 角色属性

角色 Pawn 每帧组装 Struct 供 AnimBP、Chooser、Camera、Traversal 消费。

## S_CharacterPropertiesForAnimation

- **路径**：`/Game/Blueprints/Data/S_CharacterPropertiesForAnimation`
- **引用枚举**（CDO `ScriptAndPropertyObjectReferences`）：

| 枚举资产 |
|----------|
""" + "\n".join(f"| `{e}` |" for e in anim_enums) + """

典型字段（AnimBP 变量与 Chooser 绑定一致）：

| 变量 | 用途 |
|------|------|
| `MovementMode` / `MovementMode_LastFrame` | Grounded/InAir/Slide；State Controller 顶层分流 |
| `MovementState` / `MovementState_LastFrame` | Idle/Moving；**Mover** 用于 Conduit |
| `Stance` / `Stance_LastFrame` | Stand/Crouch |
| `Gait` / `Gait_LastFrame` | Walk/Run/Sprint |
| `MovementDirection` / `MovementDirectionLastFrame` | 八方向/扇区；Re-Enter 重选 |
| `RotationMode` | 朝向模式 |
| `S_PlayerInputState` | 嵌套输入状态 |

## S_CharacterPropertiesForTraversal

- **路径**：`/Game/Blueprints/Data/S_CharacterPropertiesForTraversal`
- **引用**：`E_MovementMode`, `E_Gait`

## S_CharacterPropertiesForCamera

- **路径**：`/Game/Blueprints/Data/S_CharacterPropertiesForCamera`
- **引用**：`E_CameraStyle`, `E_CameraMode`, `E_Gait`, `E_Stance`

## Traversal Chooser Struct

### S_TraversalChooserInputs

引用：`E_TraversalActionType`, `E_MovementMode`, `E_Gait`

Bool/Float 列（Montage Chooser 绑定）：

| 字段 | 类型 |
|------|------|
| `HasFrontLedge` | bool |
| `HasBackLedge` | bool |
| `HasBackFloor` | bool |
| `ObstacleDepth` | float (cm) |
| `BackLedgeHeight` | float (cm) |

### S_TraversalChooserOutputs

输出 `E_TraversalActionType`（Hurdle/Mantle/Vault 等）。

## S_ChooserOutputs

`/Game/Blueprints/Data/S_ChooserOutputs` — MM 搜索输出（动画、标签、循环标记等）。

## S_MovementDirectionThresholds

`/Game/Blueprints/Data/S_MovementDirectionThresholds` — 方向扇区角度阈值（供 `Update_MovementDirection`）。

## Update_States 写入顺序（CMC dump）

1. `MovementMode_LastFrame` ← 旧 `MovementMode`
2. 更新 `MovementMode`
3. `IsMoving()` → 设置 `MovementState`（Idle/Moving）
4. `MovementState_LastFrame`, `Gait_LastFrame`, `Stance_LastFrame` 缓存
""")

# 13-abp-animgraph
save("13-abp-animgraph.md", role_block(
    "AnimBP 动画图与更新函数",
    "理解 AnimGraph 节点链、ThreadSafe 更新、BlendStack 时",
    "`/Game/Blueprints/SandboxCharacter_CMC_ABP`, `/Game/Blueprints/SandboxCharacter_Mover_ABP`"
) + """
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
""")

# 20-cmc-vs-mover
save("20-cmc-vs-mover.md", role_block(
    "CMC 与 Mover 双路径对比",
    "选择/迁移 locomotion 方案或对比行为差异时",
    "`/Game/Blueprints/SandboxCharacter_CMC`, `/Game/Blueprints/SandboxCharacter_Mover`"
) + """
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
""")

# 21-movement-modes
mm_walk = load("blueprints/Blueprints__MovementModes__BP_MovementMode_Walking__info.json")["result"]
mm_slide = load("blueprints/Blueprints__MovementModes__BP_MovementMode_Slide__info.json")["result"]
mm_fall = load("blueprints/Blueprints__MovementModes__BP_MovementMode_Falling__info.json")["result"]
save("21-movement-modes.md", role_block(
    "Mover MovementMode 蓝图",
    "扩展 Mover 模式、理解 Slide/Walk/Fall 切换时",
    "`/Game/Blueprints/MovementModes/BP_MovementMode_*`"
) + f"""
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

Walking 模式变量数：{mm_walk.get('variable_count', '?')}；Slide/Falling 见对应 `__info.json` dump。

## 设计要点

- MovementMode 由 Mover 组件写入，AnimBP `Update_States` 同步到 `MovementMode` 供 Chooser 与 State Controller 使用。
- Slide 过渡在 State Controller 有独立 Transition/Loop/Re-Enter（`MovementDirection` 变化触发重选）。
""")

# 30-traversal
trav = load("choosers/Characters__UEFN_Mannequin__Animations__Traversal__CHT_TraversalMontages_CMC.json")["result"]
trav_rows = trav["tree"]["rows"]
save("30-traversal.md", role_block(
    "Traversal 翻越系统",
    "调试 Hurdle/Mantle/Vault 选片与检测条件时",
    "`/Game/Characters/UEFN_Mannequin/Animations/Traversal/CHT_TraversalMontages_CMC`"
) + """
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
""")

# 40-warping-footplacement
save("40-warping-footplacement.md", role_block(
    "根运动偏移与足部放置",
    "调 Root Offset、Foot IK、Attribute RM 时",
    "AnimBP Offset Root Bone 节点；`DDCvar.FootPlacementMode`, `DDCVar.AttributeBasedRootMotion.Enable`"
) + """
# 40 — Warping 与 Foot Placement

## Offset Root Bone

| CVar | 类型 | 默认 | AnimBP 变量 |
|------|------|------|-------------|
| `DDCVar.AttributeBasedRootMotion.Enable` | Bool | false | `OffsetRootBoneEnabled` |
| `DDCvar.OffsetRootBone.TranslationRadius` | Float | 0 | `OffsetRootTranslationRadius` |

`Update_CVarDrivenVariables` 每帧缓存（Blueprint 读 CVar 非线程安全）。

**Turn in Place** 注释（`ShouldTurnInPlace`）：瞄准态下需限制 steering/root offset，避免角色滞后过多。

## Foot Placement

| CVar | 类型 | 默认 | 说明 |
|------|------|------|------|
| `DDCvar.FootPlacementMode` | Int | 1 | tooltip: `0 == off` |

AnimGraph 中 Foot Placement 节点根据 `FootPlacementMode` 启用/禁用足部落点 IK。

## Rotation Warp / Break

State Controller **Re-Enter** 过渡处理旋转动画 break：

- CMC：`TargetRotation` vs `TargetRotationOnTransitionStart`，阈值约 60°
- Mover：`Trj_FutureFacing` vs `FutureFacingOnTransitionStart`

在 start/pivot 动画初期若目标朝向快速变化，触发重选过渡动画。

## 相关动画标签

BlendStack `Contains Item` 检查 pivot/start 类标签，与 warp 时机联动（见 `14-state-controller.md`）。
""")

# 50-gameplay-camera
cam = load("choosers/Blueprints__Cameras__CHT_CameraRig.json")["result"]
cam_rows = cam["tree"]["rows"] if "rows" in cam.get("tree", {}) else []
save("50-gameplay-camera.md", role_block(
    "Gameplay Camera 系统",
    "查相机 Rig 选择逻辑与 Character 绑定时",
    "`/Game/Blueprints/Cameras/CameraAsset_SandboxCharacter`, `/Game/Blueprints/Cameras/CHT_CameraRig`"
) + f"""
# 50 — Gameplay Camera

## 核心资产

| 资产 | 路径 | 类 |
|------|------|-----|
| Camera Asset | `/Game/Blueprints/Cameras/CameraAsset_SandboxCharacter` | CameraAsset |
| Camera Director | `/Game/Blueprints/Cameras/CameraDirector_SandboxCharacter` | Blueprint |
| Rig Chooser | `/Game/Blueprints/Cameras/CHT_CameraRig` | ChooserTable → CameraRigAsset |

**引用关系**（asset_details dump）：`SandboxCharacter_CMC` 与 `SandboxCharacter_Mover` 均 Hard 引用 CameraAsset。

## CHT_CameraRig

- **Context**：`S_CharacterPropertiesForCamera`
- **输入列**：`CameraStyle`（11 行）、`CameraMode`（11 行）
- **行数**：{cam['row_count']}
- **输出类**：`/Script/GameplayCameras.CameraRigAsset`

## 启用开关

| CVar | 默认 |
|------|------|
| `DDCVar.NewGameplayCameraSystem.Enable` | true |
| `DDCvar.CameraStyle` | 1 |

## S_CharacterPropertiesForCamera 枚举引用

- `/Game/Blueprints/Cameras/E_CameraStyle`
- `/Game/Blueprints/Cameras/E_CameraMode`
- `/Game/Blueprints/Data/E_Gait`
- `/Game/Blueprints/Data/E_Stance`

CameraStyle + CameraMode 组合决定 Rig；Gait/Stance 可参与混合权重（具体 Rig 资产见 Chooser 各行输出）。
""")

# 60-smart-objects-ai
save("60-smart-objects-ai.md", role_block(
    "Smart Object 与 AI 动画",
    "查 SmartObject 动画schema或 AI 用 PSD 时",
    "`/Game/Blueprints/SmartObjects/PSS_SmartObject`（asset_index）"
) + """
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
""")

# 70-foley-retarget
save("70-foley-retarget.md", role_block(
    "Foley 音效与 Retarget",
    "同步脚步音效、换骨架 retarget 时",
    "`SK_UEFN_Mannequin`；`DDCvar.DrawVisLogShapesForFoleySounds`"
) + """
# 70 — Foley 与 Retarget

## 骨架与 Mannequin

| 资产 | 路径 |
|------|------|
| 骨架 | `/Game/Characters/UEFN_Mannequin/Meshes/SK_UEFN_Mannequin` |
| 动画根目录 | `/Game/Characters/UEFN_Mannequin/Animations/` |

所有 `PSS_*` Schema 与 `PSD_*` Database dump 均绑定此 Skeleton。

## Foley

| CVar | 默认 | 说明 |
|------|------|------|
| `DDCvar.DrawVisLogShapesForFoleySounds` | false | VisLog 可视化 foley 触发 |

Foley 通常由 AnimNotify / NotifyState 在 Montage 与 MM 选中片段上触发；具体 Notify 资产在 `/Game/Characters/UEFN_Mannequin/Animations/` 各子目录。

## Retarget 要点

1. 复制 `SK_UEFN_Mannequin` 的 IK Rig / Retargeter 设置
2. 重建或映射 `PSS_*` Schema 骨骼通道
3. 重新烘焙 `PSD_*` Database（Dense/Sparse 全层级）
4. 更新 Chooser 引用的 PSD 路径

## 动画集切换

| CVar | 默认 |
|------|------|
| `DDCVar.DefaultAnimSet` | 0 |
| `DDCvar.VisualOverride` | -1 |
| `DDCvar.UseUnrealAnimationFramework` | false |
""")

# 80-ddcvars
dd = load("config/ddcvars.json")
cvar_rows = ""
for c in dd["cvars"]:
    if c["type"] == "CVarBool":
        d = c["default_bool"]
    elif c["type"] == "CVarInt":
        d = c["default_int"]
    else:
        d = c["default_float"]
    tip = c.get("tooltip") or "—"
    cvar_rows += f"| `{c['name']}` | {c['type']} | {d} | {tip} |\n"

save("80-ddcvars.md", role_block(
    "DefaultEngine DDCvar 全集",
    "控制台调参、对齐 AnimBP 缓存变量时",
    f"来源：`Config/DefaultEngine.ini` → `_raw/config/ddcvars.json`（{dd['count']} 条）"
) + f"""
# 80 — DDCvar 控制台变量

来源：`D:\\GameAnimationSample\\Config\\DefaultEngine.ini`（dump `{dd['count']}` 条）

| CVar | 类型 | 默认值 | Tooltip |
|------|------|--------|---------|
{cvar_rows}
## 常用调参组合

| 目标 | 建议设置 |
|------|----------|
| 最密 MM | `DDCvar.MMDatabaseLOD 0` |
| 性能优先 | `DDCvar.MMDatabaseLOD 2` |
| 开 Foot IK | `DDCvar.FootPlacementMode 1` |
| 关 Foot IK | `DDCvar.FootPlacementMode 0` |
| Traversal 可视化 | `DDCvar.Traversal.DrawDebugLevel 1` |
| Experimental SM | `DDCVar.ExperimentalStateMachine.Enable 1` |
| 切 Mover 配置 | `DDCvar.LocomotionSetupMover` |
""")

# 90-asset-map
manifest = load("_manifest.json")
search = load("asset_index/search_results.json")
paths = sorted(set())
for wf in manifest.get("written_files", []):
    m = re.search(r"Knowledge\\\\_raw\\\\(.+)\\.json", wf.replace("/", "\\"))
    if m:
        paths.append(m.group(1))

# Build asset map from search results
all_assets = []
for cat, block in search.items():
    if isinstance(block, dict) and "results" in block:
        for it in block["results"]:
            if isinstance(it, dict) and "asset_path" in it:
                all_assets.append((it["asset_path"], it.get("asset_class", ""), cat))
    elif isinstance(block, list):
        for it in block:
            if isinstance(it, dict) and "asset_path" in it:
                all_assets.append((it["asset_path"], it.get("asset_class", ""), cat))

all_assets.sort()
asset_table = "| 路径 | 类 | 索引类别 |\n|------|-----|----------|\n"
seen = set()
for ap, cls, cat in all_assets[:80]:
    if ap in seen:
        continue
    seen.add(ap)
    asset_table += f"| `{ap}` | {cls} | {cat} |\n"
if not seen:
    asset_table += "| （见 search_results.json 各分类） | — | — |\n"

save("90-asset-map.md", role_block(
    "资产路径快速索引",
    "按类或路径查找 GASP 资产时",
    "`_raw/_manifest.json`, `_raw/asset_index/search_results.json`"
) + f"""
# 90 — 资产地图

## 核心入口资产

| 类 | 路径 |
|----|------|
| AnimBP CMC | `/Game/Blueprints/SandboxCharacter_CMC_ABP` |
| AnimBP Mover | `/Game/Blueprints/SandboxCharacter_Mover_ABP` |
| Pawn CMC | `/Game/Blueprints/SandboxCharacter_CMC` |
| Pawn Mover | `/Game/Blueprints/SandboxCharacter_Mover` |
| PSD 根 Chooser | `/Game/Characters/UEFN_Mannequin/Animations/MotionMatchingData/CHT_PoseSearchDatabases` |
| Traversal Chooser | `/Game/Characters/UEFN_Mannequin/Animations/Traversal/CHT_TraversalMontages_CMC` |
| Camera Asset | `/Game/Blueprints/Cameras/CameraAsset_SandboxCharacter` |
| Camera Chooser | `/Game/Blueprints/Cameras/CHT_CameraRig` |

## MovementModes

| 路径 |
|------|
| `/Game/Blueprints/MovementModes/BP_MovementMode_Walking` |
| `/Game/Blueprints/MovementModes/BP_MovementMode_Falling` |
| `/Game/Blueprints/MovementModes/BP_MovementMode_Slide` |
| `/Game/Blueprints/MovementModes/BP_MovementTransition_ToSlide` |
| `/Game/Blueprints/MovementModes/BP_MovementTransition_FromSlide` |

## Data Structs

| 路径 |
|------|
| `/Game/Blueprints/Data/S_CharacterPropertiesForAnimation` |
| `/Game/Blueprints/Data/S_CharacterPropertiesForCamera` |
| `/Game/Blueprints/Data/S_CharacterPropertiesForTraversal` |
| `/Game/Blueprints/Data/S_TraversalChooserInputs` |
| `/Game/Blueprints/Data/S_TraversalChooserOutputs` |
| `/Game/Blueprints/Data/S_ChooserOutputs` |
| `/Game/Blueprints/Data/S_PlayerInputState` |
| `/Game/Blueprints/Data/S_MovementDirectionThresholds` |

## 枚举

| 路径 |
|------|
| `/Game/Blueprints/Data/E_MovementMode` |
| `/Game/Blueprints/Data/E_MovementState` |
| `/Game/Blueprints/Data/E_Stance` |
| `/Game/Blueprints/Data/E_Gait` |
| `/Game/Blueprints/Data/E_MovementDirection` |
| `/Game/Blueprints/Data/E_RotationMode` |
| `/Game/Blueprints/Data/E_TraversalActionType` |
| `/Game/Blueprints/Cameras/E_CameraStyle` |
| `/Game/Blueprints/Cameras/E_CameraMode` |

## asset_index 摘录（前 80 条去重）

{asset_table}

## _raw dump 统计

- Manifest 写入文件数：**{manifest.get('written_count', 0)}**
- Soft failures：**{manifest.get('soft_failure_count', 0)}**（见 `_manifest.json`）
""")

# README
save("README.md", """# GASP 离线知识库（AxonGaspKB）

Epic **Game Animation Sample** 的结构化中文 Markdown 语料，供 Axon MCP / 离线 Agent 检索。

## 范围

- **仅** GASP 官方 Sample 内容
- 不含 AnimStateMachine、AnimTable、StructChooser、AnimInstanceExt 等自定义插件

## 目录

| 目录 | 说明 |
|------|------|
| `*.md`（本层） | 人类可读专题文档，编号见 `00-routing.md` |
| `_raw/` | **证据层**：Axon MCP JSON dump，**只读勿改** |

## 如何使用

1. 从 `00-routing.md` 按主题跳转
2. 深度调试 State Controller → `14-state-controller.md`
3. 结论务必能回溯 `_raw/` 中对应 JSON

## _raw 说明

- `_manifest.json`：dump 文件清单与 soft failure
- `abp/`：AnimBP 图摘要、State Controller 过渡
- `choosers/`：`inspect_chooser` 完整表结构
- `config/ddcvars.json`：DefaultEngine.ini 中全部 DDCvar
- `pose_search/`：PSS/PSD schema 与统计
- `structs/`、`blueprints/`、`asset_details/`：CDO 与元数据

**请勿编辑 `_raw/`**；若引擎资产变更，应重新运行 dump 脚本生成新证据后再更新上层 Markdown。
""")

# Report
print("Written files:")
for name, lines in sorted(written):
    print(f"  {name}: ~{lines} lines")
print(f"Total: {len(written)} files")
