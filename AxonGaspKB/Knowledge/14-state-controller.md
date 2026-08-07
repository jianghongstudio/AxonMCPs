> **角色**：State Controller 状态机权威参考  
> **何时阅读**：调试 locomotion 过渡、Re-Enter、pivot、Slide（Mover）逻辑时**必读**  
> **相关资产**：`/Game/Blueprints/SandboxCharacter_CMC_ABP`, `/Game/Blueprints/SandboxCharacter_Mover_ABP`

# 14 — State Controller

AnimGraph 内 **State Controller** 状态机为纯逻辑控制器，驱动 BlendStack 选片，不直接持有动画资产。开关：`DDCVar.ExperimentalStateMachine.Enable`（默认 False；见 `80-ddcvars.md`）。

## 外壳拓扑（Experimental 路径）

官方 AnimGraph 注释主题：**State Machine + Choosers + Motion Matching + Blend Stack (Experimental)**。

| 节点 | 作用 |
|------|------|
| `State Controller` | 逻辑态 SM；pose 不作为最终输出 |
| `Inertialization` | 接在 SM 之后（A 侧） |
| `Two Way Blend` | Alpha=**1.0** 只取 B 侧（Blend Stack）；**Always Update Children** 保证 A 侧 SM 每帧仍先跑逻辑 |
| `State Machine Blend Stack` | 真正出 pose；输入绑定 `Blend Stack Inputs`（Anim / Start Time / Loop / Blend Time / Blend Profile，Thread Safe） |
| On State Entry 函数 | 如 `OnStateEntry_IdleLoop`… 调用 `SetBlendStackAnimFromChooser` 写入 Blend Stack Inputs |

数据流：`On State Entry` → Chooser/MM 选片 → 写 `BlendStackInputs` → Blend Stack 出 pose；SM 边条件决定何时进入 Transition/Loop/Re-Enter。

## 状态清单对比

| 状态 | CMC (7) | Mover (9) |
|------|-----|-------|
| `Idle Break` | ✓ | ✓ |
| `Idle Loop` | ✓ | ✓ |
| `In Air Loop` | ✓ | ✓ |
| `Locomotion Loop` | ✓ | ✓ |
| `Slide Loop` | — | ✓ |
| `Transition to Idle` | ✓ | ✓ |
| `Transition to In Air` | ✓ | ✓ |
| `Transition to Locomotion` | ✓ | ✓ |
| `Transition to Slide` | — | ✓ |

- **入口状态**：两者均为 `Transition to Idle`（`_raw/.../state_machines.json`）
- **CMC 过渡数**：22
- **Mover 过渡数**：29（含 Slide 与 Mover 特有 Re-Enter 规则）

## CMC — `/Game/Blueprints/SandboxCharacter_CMC_ABP`

## 完整过渡表

来源：`_raw/abp/.../transitions_StateController.json`，AnimBP `/Game/Blueprints/SandboxCharacter_CMC_ABP`

| # | 源 | 目标 | 类型 | CrossFade | 条件摘要 | 关键变量/函数 |
|---|-----|------|------|-----------|----------|---------------|
| 1 | `Idle -> Locomotion` | `Transition to Locomotion` | state→state | 0.20s | 角色正在移动（`IsMoving()`：Vel∧TrjFutureVel∧Accel 均非零，AND） | `Is Moving` |
| 2 | `Locomotion -> Idle` | `Transition to Idle` | state→state | 0.20s | 角色正在移动（`IsMoving()`：Vel∧TrjFutureVel∧Accel 均非零，AND）；取反 | `Is Moving`, `NOT Boolean` |
| 3 | `Transition to Locomotion` | `Locomotion Loop` | state→state | 0.00s | BlendStack 中当前动画接近播放完毕；`CurrentStateTime` 阈值判断。This acts simliar to the "Automatic Rule" condition in typical state machines, which triggers the transition automatically whenever the animation is almost over. Since this state machine is purely log | `Is Animation Almost Complete`, `Current State Time (State Controller)` |
| 4 | `Transition to Locomotion` | `Locomotion Loop` | state→state | 0.00s | `NoValidAnim`；`BlendStackInputs`；解析 `BlendStackInputs`。If no animations were found when entering into the transition state, this transition takes us straight to the looping state.  In addition, if a looping animation was chosen when searching for transiti | `NoValidAnim`, `BlendStackInputs`, `Break S Blend Stack Inputs` |
| 5 | `Transition to Idle` | `Idle Loop` | state→state | 0.00s | BlendStack 中当前动画接近播放完毕；`CurrentStateTime` 阈值判断。This acts simliar to the "Automatic Rule" condition in typical state machines, which triggers the transition automatically whenever the animation is almost over. Since this state machine is purely log | `Is Animation Almost Complete`, `Current State Time (State Controller)` |
| 6 | `Transition to Idle` | `Idle Loop` | state→state | 0.00s | `NoValidAnim`；`BlendStackInputs`；解析 `BlendStackInputs`。If no animations were found when entering into the transition state, this transition takes us straight to the looping state.  In addition, if a looping animation was chosen when searching for transiti | `NoValidAnim`, `BlendStackInputs`, `Break S Blend Stack Inputs` |
| 7 | `Transition to In Air` | `In Air Loop` | state→state | 0.00s | BlendStack 中当前动画接近播放完毕；`CurrentStateTime` 阈值判断。This acts simliar to the "Automatic Rule" condition in typical state machines, which triggers the transition automatically whenever the animation is almost over. Since this state machine is purely log | `Is Animation Almost Complete`, `Current State Time (State Controller)` |
| 8 | `Transition to In Air` | `In Air Loop` | state→state | 0.00s | `NoValidAnim`；`BlendStackInputs`；解析 `BlendStackInputs`。If no animations were found when entering into the transition state, this transition takes us straight to the looping state.  In addition, if a looping animation was chosen when searching for transiti | `NoValidAnim`, `BlendStackInputs`, `Break S Blend Stack Inputs` |
| 9 | `-> In Air` | `Transition to In Air` | state→state | 0.20s | `MovementMode` | `MovementMode` |
| 10 | `-> Grounded` | `Conduit` | state→conduit | 0.20s | `MovementMode` | `MovementMode` |
| 11 | `Re-Enter` | `Transition to Locomotion` | state→state | 0.20s | `Stance`；`Stance_LastFrame`；`MovementDirection`；`MovementDirectionLastFrame`；`Gait`；`Gait_LastFrame`；`CurrentStateTime` 阈值判断。If any of these states have changed, we know we need to reselect a locomotion animation. Therefore, transition to (or re-start) the "Transition to Locomotion Loop" state.  Checking to see if the curre | `Stance`, `Stance_LastFrame`, `MovementDirection`, `MovementDirectionLastFrame`, `Gait`, `Gait_LastFrame`, `Current State Time (State Controller)` |
| 12 | `Re-Enter` | `Transition to Locomotion` | state→state | 0.20s | `CurrentStateTime` 阈值判断；`BlendStackInputs`；解析 `BlendStackInputs`；正在 pivot（`IsPivoting()`）。If "Is Pivoting" is true, and we are not playing the beginning of a pivot or start animation, then transition into the "Transition to Locomotion Loop" state. Since "Is Pivoting" is also used in the ch | `Current State Time (State Controller)`, `BlendStackInputs`, `Break S Blend Stack Inputs`, `Is Pivoting` |
| 13 | `Re-Enter` | `Transition to Locomotion` | state→state | 0.20s | `TargetRotation`；`TargetRotationOnTransitionStart`；Rotator 角度差；取绝对值；`CurrentStateTime` 阈值判断；`BlendStackInputs`；解析 `BlendStackInputs`。This transition handles the concept of "breaking" a rotational animation.  Sometimes, right after a start or a pivot is triggered, the Target Rotation changes rapidly on the following frames. For inst | `TargetRotation`, `TargetRotationOnTransitionStart`, `Delta (Rotator)`, `Absolute (Float)`, `Current State Time (State Controller)`, `BlendStackInputs`, `Break S Blend Stack Inputs` |
| 14 | `Re-Enter ` | `Transition to Idle` | state→state | 0.20s | `Stance`；`Stance_LastFrame`；`CurrentStateTime` 阈值判断。If any of these states have changed, we know we need to reselect an idle animation. Therefore, transition to (or re-start) the "Transition to Idle Loop" state.  Checking to see if the current state ti | `Stance`, `Stance_LastFrame`, `Current State Time (State Controller)` |
| 15 | `Idle Loop` | `Idle Break` | state→state | 0.20s | `CurrentStateTime` 阈值判断 | `Current State Time (State Controller)` |
| 16 | `Idle Break` | `Idle Loop` | state→state | 0.20s | BlendStack 中当前动画接近播放完毕；`CurrentStateTime` 阈值判断。This acts simliar to the "Automatic Rule" condition in typical state machines, which triggers the transition automatically whenever the animation is almost over. Since this state machine is purely log | `Is Animation Almost Complete`, `Current State Time (State Controller)` |
| 17 | `Idle Break` | `Idle Loop` | state→state | 0.20s | `NoValidAnim`；`BlendStackInputs`；解析 `BlendStackInputs`。If no animations were found when entering into the transition state, this transition takes us straight to the looping state.  In addition, if a looping animation was chosen when searching for transiti | `NoValidAnim`, `BlendStackInputs`, `Break S Blend Stack Inputs` |
| 18 | `Re-Enter ` | `Transition to Idle` | state→state | 0.20s | 应原地转身（`ShouldTurnInPlace()`）；`CurrentStateTime` 阈值判断；`BlendStackInputs`；解析 `BlendStackInputs`。This transition is very simliar to the pivot transition in the locomotion states. If "Should Turn in Place" is true, and we are not playing the beginning of a TurnInPlace animation, then transition in | `Should Turn in Place`, `Current State Time (State Controller)`, `BlendStackInputs`, `Break S Blend Stack Inputs` |
| 19 | `Conduit` | `Transition to Locomotion` | conduit→state | 0.20s | 角色正在移动（`IsMoving()`：Vel∧TrjFutureVel∧Accel 均非零，AND） | `Is Moving` |
| 20 | `Conduit` | `Transition to Idle` | conduit→state | 0.20s | 角色正在移动（`IsMoving()`：Vel∧TrjFutureVel∧Accel 均非零，AND）；取反 | `Is Moving`, `NOT Boolean` |
| 21 | `Re-Enter` | `Transition to Locomotion` | state→state | 0.20s | `NotifyTransition_Re-Transition`。This condition is set using the "BP_NotifyState_EarlyTransition" notify state. This notify state allows us to transition out of animations early depending on the conditions in the notify. | `NotifyTransition_Re-Transition` |
| 22 | `Transition to Locomotion` | `Locomotion Loop` | state→state | 0.20s | `NotifyTransition_ToLoop`。This condition is set using the "BP_NotifyState_EarlyTransition" notify state. This notify state allows us to transition out of animations early depending on the conditions in the notify. | `NotifyTransition_ToLoop` |

## Mover — `/Game/Blueprints/SandboxCharacter_Mover_ABP`

## 完整过渡表

来源：`_raw/abp/.../transitions_StateController.json`，AnimBP `/Game/Blueprints/SandboxCharacter_Mover_ABP`

| # | 源 | 目标 | 类型 | CrossFade | 条件摘要 | 关键变量/函数 |
|---|-----|------|------|-----------|----------|---------------|
| 1 | `Idle -> Locomotion` | `Transition to Locomotion` | state→state | 0.20s | `MovementState` | `MovementState` |
| 2 | `Locomotion -> Idle` | `Transition to Idle` | state→state | 0.20s | `MovementState` | `MovementState` |
| 3 | `Transition to Locomotion` | `Locomotion Loop` | state→state | 0.20s | BlendStack 中当前动画接近播放完毕；`CurrentStateTime` 阈值判断。This acts simliar to the "Automatic Rule" condition in typical state machines, which triggers the transition automatically whenever the animation is almost over. Since this state machine is purely log | `Is Animation Almost Complete`, `Current State Time (State Controller)` |
| 4 | `Transition to Locomotion` | `Locomotion Loop` | state→state | 0.20s | `NoValidAnim`；`BlendStackInputs`；解析 `BlendStackInputs`。If no animations were found when entering into the transition state, this transition takes us straight to the looping state.  In addition, if a looping animation was chosen when searching for transiti | `NoValidAnim`, `BlendStackInputs`, `Break S Blend Stack Inputs` |
| 5 | `Transition to Idle` | `Idle Loop` | state→state | 0.20s | BlendStack 中当前动画接近播放完毕；`CurrentStateTime` 阈值判断。This acts simliar to the "Automatic Rule" condition in typical state machines, which triggers the transition automatically whenever the animation is almost over. Since this state machine is purely log | `Is Animation Almost Complete`, `Current State Time (State Controller)` |
| 6 | `Transition to Idle` | `Idle Loop` | state→state | 0.20s | `NoValidAnim`；`BlendStackInputs`；解析 `BlendStackInputs`。If no animations were found when entering into the transition state, this transition takes us straight to the looping state.  In addition, if a looping animation was chosen when searching for transiti | `NoValidAnim`, `BlendStackInputs`, `Break S Blend Stack Inputs` |
| 7 | `Transition to In Air` | `In Air Loop` | state→state | 0.20s | BlendStack 中当前动画接近播放完毕；`CurrentStateTime` 阈值判断。This acts simliar to the "Automatic Rule" condition in typical state machines, which triggers the transition automatically whenever the animation is almost over. Since this state machine is purely log | `Is Animation Almost Complete`, `Current State Time (State Controller)` |
| 8 | `Transition to In Air` | `In Air Loop` | state→state | 0.20s | `NoValidAnim`；`BlendStackInputs`；解析 `BlendStackInputs`。If no animations were found when entering into the transition state, this transition takes us straight to the looping state.  In addition, if a looping animation was chosen when searching for transiti | `NoValidAnim`, `BlendStackInputs`, `Break S Blend Stack Inputs` |
| 9 | `-> In Air` | `Transition to In Air` | state→state | 0.20s | `MovementMode` | `MovementMode` |
| 10 | `-> Grounded` | `Conduit` | state→conduit | 0.20s | `MovementMode` | `MovementMode` |
| 11 | `Re-Enter` | `Transition to Locomotion` | state→state | 0.20s | `Stance`；`Stance_LastFrame`；`MovementDirection`；`MovementDirection_LastFrame`；`Gait`；`Gait_LastFrame`；`CurrentStateTime` 阈值判断。If any of these states have changed, we know we need to reselect a locomotion animation. Therefore, transition to (or re-start) the "Transition to Locomotion Loop" state.  Checking to see if the curre | `Stance`, `Stance_LastFrame`, `MovementDirection`, `MovementDirection_LastFrame`, `Gait`, `Gait_LastFrame`, `Current State Time (State Controller)` |
| 12 | `Re-Enter` | `Transition to Locomotion` | state→state | 0.20s | `CurrentStateTime` 阈值判断；`BlendStackInputs`；正在 pivot（`IsPivoting()`）；解析 `BlendStackInputs`；`Trj_IsCircling`；取反；`MovementMode_Recent`。If "Is Pivoting" is true, and we are not playing the beginning of a pivot or start animation, then transition into the "Transition to Locomotion Loop" state. Since "Is Pivoting" is also used in the ch | `Current State Time (State Controller)`, `BlendStackInputs`, `Is Pivoting`, `Break S Blend Stack Inputs`, `Trj_IsCircling`, `NOT Boolean`, `MovementMode_Recent` |
| 13 | `Re-Enter` | `Transition to Locomotion` | state→state | 0.20s | `Trj_FutureFacing`；`FutureFacingOnTransitionStart`；Rotator 角度差；取绝对值；`CurrentStateTime` 阈值判断；`BlendStackInputs`；解析 `BlendStackInputs`；取反。This transition handles the concept of "breaking" a rotational animation.  Sometimes, right after a start or a pivot is triggered, the Target Rotation changes rapidly on the following frames. For inst | `Trj_FutureFacing`, `FutureFacingOnTransitionStart`, `Delta (Rotator)`, `Absolute (Float)`, `Current State Time (State Controller)`, `BlendStackInputs`, `Break S Blend Stack Inputs`, `NOT Boolean`, `Trj_IsCircling`, `NOT Boolean` |
| 14 | `Re-Enter ` | `Transition to Idle` | state→state | 0.20s | `Stance`；`Stance_LastFrame`；`CurrentStateTime` 阈值判断。If any of these states have changed, we know we need to reselect an idle animation. Therefore, transition to (or re-start) the "Transition to Idle Loop" state.  Checking to see if the current state ti | `Stance`, `Stance_LastFrame`, `Current State Time (State Controller)` |
| 15 | `Idle Loop` | `Idle Break` | state→state | 0.20s | `CurrentStateTime` 阈值判断 | `Current State Time (State Controller)` |
| 16 | `Idle Break` | `Idle Loop` | state→state | 0.20s | BlendStack 中当前动画接近播放完毕；`CurrentStateTime` 阈值判断。This acts simliar to the "Automatic Rule" condition in typical state machines, which triggers the transition automatically whenever the animation is almost over. Since this state machine is purely log | `Is Animation Almost Complete`, `Current State Time (State Controller)` |
| 17 | `Idle Break` | `Idle Loop` | state→state | 0.20s | `NoValidAnim`；`BlendStackInputs`；解析 `BlendStackInputs`。If no animations were found when entering into the transition state, this transition takes us straight to the looping state.  In addition, if a looping animation was chosen when searching for transiti | `NoValidAnim`, `BlendStackInputs`, `Break S Blend Stack Inputs` |
| 18 | `Re-Enter ` | `Transition to Idle` | state→state | 0.20s | 应原地转身（`ShouldTurnInPlace()`）；`CurrentStateTime` 阈值判断；`BlendStackInputs`；解析 `BlendStackInputs`；`FutureFacingDelta`；`BlendStackInputs`；解析 `BlendStackInputs`；`FutureFacingDelta`。This transition is very simliar to the pivot transition in the locomotion states. If "Should Turn in Place" is true, and we are not playing the beginning of a TurnInPlace animation, then transition in | `Should Turn in Place`, `Current State Time (State Controller)`, `BlendStackInputs`, `Break S Blend Stack Inputs`, `FutureFacingDelta`, `BlendStackInputs`, `Break S Blend Stack Inputs`, `FutureFacingDelta`, `Curve Value` |
| 19 | `Conduit` | `Transition to Locomotion` | conduit→state | 0.20s | `MovementState` | `MovementState` |
| 20 | `Conduit` | `Transition to Idle` | conduit→state | 0.20s | `MovementState` | `MovementState` |
| 21 | `Re-Enter` | `Transition to Locomotion` | state→state | 0.20s | `NotifyTransition_Re-Transition`。This condition is set using the "BP_NotifyState_EarlyTransition" notify state. This notify state allows us to transition out of animations early depending on the conditions in the notify. | `NotifyTransition_Re-Transition` |
| 22 | `Transition to Locomotion` | `Locomotion Loop` | state→state | 0.20s | `NotifyTransition_ToLoop`。This condition is set using the "BP_NotifyState_EarlyTransition" notify state. This notify state allows us to transition out of animations early depending on the conditions in the notify. | `NotifyTransition_ToLoop` |
| 23 | `Transition to Slide` | `Slide Loop` | state→state | 0.20s | BlendStack 中当前动画接近播放完毕；`CurrentStateTime` 阈值判断。This acts simliar to the "Automatic Rule" condition in typical state machines, which triggers the transition automatically whenever the animation is almost over. Since this state machine is purely log | `Is Animation Almost Complete`, `Current State Time (State Controller)` |
| 24 | `Transition to Slide` | `Slide Loop` | state→state | 0.20s | `NoValidAnim`；`BlendStackInputs`；解析 `BlendStackInputs`。If no animations were found when entering into the transition state, this transition takes us straight to the looping state.  In addition, if a looping animation was chosen when searching for transiti | `NoValidAnim`, `BlendStackInputs`, `Break S Blend Stack Inputs` |
| 25 | `-> Slide` | `Transition to Slide` | state→state | 0.20s | `MovementMode` | `MovementMode` |
| 26 | `Re-Enter  ` | `Transition to Slide` | state→state | 0.20s | `MovementDirection`；`MovementDirection_LastFrame`；`CurrentStateTime` 阈值判断 | `MovementDirection`, `MovementDirection_LastFrame`, `Current State Time (State Controller)` |
| 27 | `Transition to Locomotion` | `Locomotion Loop` | state→state | 0.20s | `Trj_CirclingTime`；`CurrentStateTime` 阈值判断；`BlendStackInputs`；解析 `BlendStackInputs` | `Trj_CirclingTime`, `Current State Time (State Controller)`, `BlendStackInputs`, `Break S Blend Stack Inputs` |
| 28 | `Re-Enter` | `Transition to Locomotion` | state→state | 0.20s | 取绝对值；`FutureFacingDelta`；`FutureFacingDelta_LastFrame`；`CurrentStateTime` 阈值判断 | `Absolute (Float)`, `FutureFacingDelta`, `FutureFacingDelta_LastFrame`, `Current State Time (State Controller)` |
| 29 | `Re-Enter   ` | `Transition to In Air` | state→state | 0.20s | `MovementDirection`；`MovementDirection_LastFrame`；`CurrentStateTime` 阈值判断 | `MovementDirection`, `MovementDirection_LastFrame`, `Current State Time (State Controller)` |

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
