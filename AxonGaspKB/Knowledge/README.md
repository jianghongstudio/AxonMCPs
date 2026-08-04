# GASP 离线知识库（AxonGaspKB）

Epic **Game Animation Sample** 的结构化中文 Markdown 语料，供 Axon MCP / 离线 Agent 检索。

本插件是 Axon **项目蒸馏**能力的成品示例：薄壳注册 namespace `gasp_kb`，机制来自 sibling **AxonKnowledgeLib**。若要为**其他工程**生成同类包，对 Agent 说「蒸馏本项目」（见 `Axon/.Knowledges/31-knowledge-distill.md`），不要为 GASP 再 scaffold 一套新名字。

## 范围

- **仅** GASP 官方 Sample 内容
- 不含 AnimStateMachine、AnimTable、StructChooser、AnimInstanceExt 等自定义插件

## 目录

| 目录 | 说明 |
|------|------|
| `*.md`（本层） | 人类可读专题文档，编号见 `00-routing.md` |
| `_raw/` | **证据层**：Axon MCP JSON dump，**只读勿改** |

## 如何使用

1. MCP：`gasp_kb_query` → `route` / `search` / `read` / `list_topics`
2. 从 `00-routing.md` 按主题跳转
3. 深度调试 State Controller → `14-state-controller.md`
4. 结论务必能回溯 `_raw/` 中对应 JSON

## _raw 说明

- `_manifest.json`：dump 文件清单与 soft failure
- `abp/`：AnimBP 图摘要、State Controller 过渡
- `choosers/`：`inspect_chooser` 完整表结构
- `config/ddcvars.json`：DefaultEngine.ini 中全部 DDCvar
- `pose_search/`：PSS/PSD schema 与统计
- `structs/`、`blueprints/`、`asset_details/`：CDO 与元数据

**请勿编辑 `_raw/`**；若引擎资产变更，应重新运行 dump 脚本生成新证据后再更新上层 Markdown。
