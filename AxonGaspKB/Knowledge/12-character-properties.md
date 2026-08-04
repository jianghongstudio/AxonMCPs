> **角色**：角色属性 Struct 与 ABP 变量  
> **何时阅读**：查 MovementMode/Gait/Stance 来源与 Chooser 输入绑定时  
> **相关资产**：`/Game/Blueprints/Data/S_CharacterPropertiesFor*`

# 12 — 角色属性

角色 Pawn 每帧组装 Struct 供 AnimBP、Chooser、Camera、Traversal 消费。

## S_CharacterPropertiesForAnimation

- **路径**：`/Game/Blueprints/Data/S_CharacterPropertiesForAnimation`
- **引用枚举**（CDO `ScriptAndPropertyObjectReferences`）：

| 枚举资产 |
|----------|
| `/Game/Blueprints/Data/S_PlayerInputState.S_PlayerInputState` |
| `/Game/Blueprints/Data/E_MovementMode.E_MovementMode` |
| `/Game/Blueprints/Data/E_Stance.E_Stance` |
| `/Game/Blueprints/Data/E_RotationMode.E_RotationMode` |
| `/Game/Blueprints/Data/E_Gait.E_Gait` |
| `/Game/Blueprints/Data/E_MovementDirection.E_MovementDirection` |

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
