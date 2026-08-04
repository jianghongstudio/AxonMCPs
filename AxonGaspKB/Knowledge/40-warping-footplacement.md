> **角色**：根运动偏移与足部放置  
> **何时阅读**：调 Root Offset、Foot IK、Attribute RM 时  
> **相关资产**：AnimBP Offset Root Bone 节点；`DDCvar.FootPlacementMode`, `DDCVar.AttributeBasedRootMotion.Enable`

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
