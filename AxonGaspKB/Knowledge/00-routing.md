> **角色**：知识库导航索引  
> **何时阅读**：首次打开本语料库时；不确定该读哪篇文档时  
> **相关资产**：全部 `/Game/...` 资产（见各专题文档）

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
