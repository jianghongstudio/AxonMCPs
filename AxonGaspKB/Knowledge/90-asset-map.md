> **角色**：资产路径快速索引  
> **何时阅读**：按类或路径查找 GASP 资产时  
> **相关资产**：`_raw/_manifest.json`, `_raw/asset_index/search_results.json`

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

| 路径 | 类 | 索引类别 |
|------|-----|----------|
| `/Game/Blueprints/Cameras/CHT_CameraRig` | ChooserTable | camera |
| `/Game/Blueprints/Cameras/CameraAsset_SandboxCharacter` | CameraAsset | camera |
| `/Game/Blueprints/Cameras/CameraDirector_SandboxCharacter` | Blueprint | camera |
| `/Game/Blueprints/Data/E_MovementMode` | UserDefinedEnum | movement_modes |
| `/Game/Blueprints/MovementModes/BP_MovementMode_Falling` | Blueprint | movement_modes |
| `/Game/Blueprints/MovementModes/BP_MovementMode_Slide` | Blueprint | movement_modes |
| `/Game/Blueprints/MovementModes/BP_MovementMode_Walking` | Blueprint | movement_modes |
| `/Game/Blueprints/MovementModes/BP_MovementTransition_FromSlide` | Blueprint | movement_modes |
| `/Game/Blueprints/MovementModes/BP_MovementTransition_ToSlide` | Blueprint | movement_modes |
| `/Game/Blueprints/SandboxCharacter_CMC` | Blueprint | movement_modes |
| `/Game/Blueprints/SandboxCharacter_CMC_ABP` | AnimBlueprint | movement_modes |
| `/Game/Blueprints/SandboxCharacter_Mover` | Blueprint | movement_modes |
| `/Game/Blueprints/SandboxCharacter_Mover_ABP` | AnimBlueprint | movement_modes |
| `/Game/Blueprints/SmartObjects/PSS_SmartObject` | PoseSearchSchema | pose_search_schema |
| `/Game/Characters/UEFN_Mannequin/Animations/ExperimentalStateMachineData/PSD_SM_CMC_Idles` | PoseSearchDatabase | pose_search_database |


## _raw dump 统计

- Manifest 写入文件数：**89**
- Soft failures：**4**（见 `_manifest.json`）
