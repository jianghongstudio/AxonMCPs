> **角色**：Foley 音效与 Retarget  
> **何时阅读**：同步脚步音效、换骨架 retarget 时  
> **相关资产**：`SK_UEFN_Mannequin`；`DDCvar.DrawVisLogShapesForFoleySounds`

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
