> **角色**：DefaultEngine DDCvar 全集  
> **何时阅读**：控制台调参、对齐 AnimBP 缓存变量时  
> **相关资产**：来源：`Config/DefaultEngine.ini` → `_raw/config/ddcvars.json`（25 条）

# 80 — DDCvar 控制台变量

来源：`D:\GameAnimationSample\Config\DefaultEngine.ini`（dump `25` 条）

| CVar | 类型 | 默认值 | Tooltip |
|------|------|--------|---------|
| `DDCvar.Traversal.DrawDebugLevel` | CVarInt | 0 | — |
| `DDCvar.Traversal.DrawDebugDuration` | CVarFloat | 1.0 | — |
| `DDCvar.MMDatabaseLOD` | CVarInt | 0 | — |
| `DDCvar.DrawVisLogShapesForFoleySounds` | CVarBool | False | — |
| `DDCVar.AttributeBasedRootMotion.Enable` | CVarBool | False | — |
| `DDCvar.OffsetRootBone.TranslationRadius` | CVarFloat | 0.0 | — |
| `DDCVar.ExperimentalStateMachine.Enable` | CVarBool | False | — |
| `DDCVar.ExperimentalStateMachine.Debug` | CVarBool | False | — |
| `DDCVar.ThreadSafeAnimationUpdate.Enable` | CVarBool | True | — |
| `DDCVar.NewGameplayCameraSystem.Enable` | CVarBool | True | — |
| `DDCvar.LocomotionSetupCMC` | CVarInt | 0 | — |
| `DDCvar.LocomotionSetupMover` | CVarInt | 1 | — |
| `DDCVar.DefaultAnimSet` | CVarInt | 0 | — |
| `DDCvar.StrafeStyle` | CVarInt | 1 | — |
| `DDCvar.AimStyle` | CVarInt | 0 | — |
| `DDCvar.CameraStyle` | CVarInt | 1 | — |
| `DDCvar.UseUnrealAnimationFramework` | CVarBool | False | — |
| `DDCvar.DrawCharacterDebugShapes` | CVarBool | False | — |
| `DDCvar.DrawCharacterDebugStates` | CVarBool | False | — |
| `DDCvar.DrawCharacterDebugGraphs` | CVarBool | False | — |
| `DDCvar.FootPlacementMode` | CVarInt | 1 | 0=关, 1=Foot Placement 节点, 2=Biped Control Rig |
| `DDCvar.ControlStyle` | CVarInt | 0 | — |
| `DDCvar.PawnClass` | CVarInt | -1 | — |
| `DDCvar.VisualOverride` | CVarInt | -1 | — |
| `DDCvar.AnalogInputStyle` | CVarInt | 0 | — |

## 常用调参组合

| 目标 | 建议设置 |
|------|----------|
| 最密 MM | `DDCvar.MMDatabaseLOD 0` |
| 性能优先 | `DDCvar.MMDatabaseLOD 2` |
| 开 Foot IK | `DDCvar.FootPlacementMode 1` |
| 关 Foot IK | `DDCvar.FootPlacementMode 0` |
| Traversal 可视化 | `DDCvar.Traversal.DrawDebugLevel 1` |
| Experimental SM | `DDCVar.ExperimentalStateMachine.Enable 1` |
| 默认 Mover（工程默认） | `DDCvar.LocomotionSetupMover 1`，`DDCvar.LocomotionSetupCMC 0` |
| 切 CMC | `DDCvar.LocomotionSetupCMC 1`，`DDCvar.LocomotionSetupMover 0` |
