> **角色**：Gameplay Camera 系统  
> **何时阅读**：查相机 Rig 选择逻辑与 Character 绑定时  
> **相关资产**：`/Game/Blueprints/Cameras/CameraAsset_SandboxCharacter`, `/Game/Blueprints/Cameras/CHT_CameraRig`

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
- **行数**：11
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
