# Axon 用户与开发者指南

Axon 是一套跑在 **Unreal Editor** 里的 MCP 服务，面向游戏 **3C（角色 / 镜头 / 操控）** 的制作与验证：让 Cursor 等 Agent 通过工具调用完成脚手架、PIE 抽检、动画/GAS/蓝图 authoring，而不必人工点遍编辑器。

还支持：

- **资产 / 源码索引**：AxonIndex（`project_query`）、AxonSource（`source_query`）
- **项目蒸馏**：把工程抽成独立离线知识包（`Axon{工程名}KB`），无源 Content 也可检索蒸馏文档

更细约定见插件知识库：[`.Knowledges/`](../.Knowledges/)（入口 [`../README.md`](../README.md)）。索引专页：[32-index-and-source.md](../.Knowledges/32-index-and-source.md)；蒸馏专页：[31-knowledge-distill.md](../.Knowledges/31-knowledge-distill.md)。3C 操作链见 [3C_WORKFLOWS.md](3C_WORKFLOWS.md)。

## 1. 你需要什么

- 启用插件：**Axon**（必选），以及按需的 sibling：
  - **AxonAnimation** — 动画 / Chooser / PoseSearch
  - **AxonBlueprint** — 蓝图与 Motion Matching 脚手架
  - **AxonGAS** — Gameplay Abilities
  - **AxonConfig** — 配置查询
  - **AxonRewindDebugger** — 回放采样（镜头相关需项目里的 CameraBlueprint）
  - **AxonIndex** — 项目资产 FTS / 引用图（`project_query`）
  - **AxonSource** — 引擎与工程 C++/Shader 索引（`source_query`；首次需 reindex）
  - **AxonKnowledgeLib** — 项目蒸馏脚手架与 KB 共享库（要「蒸馏本项目」时必选）
  - **AxonGaspKB**（可选）— Epic Game Animation Sample 的离线知识示例（`gasp_kb`）
- 编辑器已启动（MCP 挂在编辑器进程内，不是独立 exe）
- MCP 客户端（Cursor 等）能访问本机 HTTP

## 2. 连接 MCP

默认地址：

```text
http://localhost:9320/mcp
```

示例配置（亦见 [`../Templates/.mcp.json.example`](../Templates/.mcp.json.example)）：

```json
{
  "mcpServers": {
    "axon": {
      "type": "http",
      "url": "http://localhost:9320/mcp"
    }
  }
}
```

**重要：** 本仓库根目录的 `.mcp.json` 可能仍指向 **Monolith**（常见端口 **9316**）。那是另一套 MCP。要用 Axon，请确认 URL 为 **9320**，或同时配置两个 server。

### 设置项

**Edit → Project Settings → Plugins → Axon**

| 项 | 默认 | 说明 |
|----|------|------|
| Enable MCP Server | true | 总开关 |
| Server Port | 9320 | 与客户端 URL 一致 |

改端口后需重启服务（重启编辑器，或使用控制台 `Axon.Restart`，以当前构建为准）。状态栏可看到 Axon 监听/连接状态。

## 3. 工具怎么用（概念）

Axon 把能力分成 **namespace + action**。客户端上通常看到：

| 客户端工具 | 含义 |
|------------|------|
| `axon_discover` / `axon_status` / `axon_guide` | 发现、健康、指南 |
| `editor_query` | 编辑器 / PIE 域，参数里带 `action` |
| `animation_query` / `blueprint_query` / `gas_query` / … | 各 sibling |
| `project_query` | 项目资产索引（AxonIndex）：搜资产、引用、Tag |
| `source_query` | 源码索引（AxonSource）：签名、调用方、搜 C++/Shader |
| `knowledge_query` | 蒸馏脚手架：`preview_kb_names` / `scaffold_kb_plugin` |
| `{project}_kb_query` / `gasp_kb_query` | 某工程离线知识包：search / read / extract… |
| `describe_query` / `bulk_fill_query` | 查 schema / 反射批量写入 |

示例：启动 PIE smoke（逻辑等价于内部 `editor.run_pie_smoke`）：

```json
{
  "action": "run_pie_smoke",
  "map": "/Game/YourMap",
  "duration": 5,
  "sample_vars": ["GroundSpeed", "bShouldMove"]
}
```

通过 `editor_query` 传入上述字段。然后用 `action: "poll_pie_smoke"` 与返回的 `session_id` 轮询。

完整参数以运行时为准：

1. `axon_discover` 看有哪些 action  
2. `describe_query` + `action: "action_schema"` 看某一条的参数

## 4. 第一次 5 分钟

1. 开编辑器，确认状态栏 Axon 在监听。  
2. 客户端调用 `axon_status`（端口、uptime、action 数）。  
3. `axon_discover` 应能看到 `editor`、`animation` 等（取决于已启用 sibling）。  
4. 若只要验证连通：`editor_query` → `action: "get_viewport_info"`（或任意只读 action）。  
5. 要做 3C 验证：打开 [3C_WORKFLOWS.md](3C_WORKFLOWS.md)。

## 5. 资产索引与源码索引（AxonIndex / AxonSource）

| 需求 | 启用 | 工具 | 说明 |
|------|------|------|------|
| 全项目搜蓝图/资产、查谁引用了谁、GameplayTag | **AxonIndex** | `project_query` | 编辑器启动后**自动建索引**（`Saved/ProjectIndex.db`） |
| 查引擎/工程 C++ API、调用方、`#include`、用法示例 | **AxonSource** | `source_query` | **不会**启动时全量索引；第一次先 `trigger_reindex`（`Saved/EngineSource.db`） |

示例（概念）：

```json
// 搜项目资产
{ "action": "search", "query": "StateController" }

// 查依赖
{ "action": "find_references", "asset_path": "/Game/..." }

// 源码：先建库（仅首次或引擎大变）
{ "action": "trigger_reindex" }

// 再查签名 / 调用方
{ "action": "get_signature", "symbol": "UCharacterMovementComponent::..." }
{ "action": "find_callers", "symbol": "..." }
```

通过 `project_query` / `source_query` 传入上述字段。完整参数用 `describe_query` + `action_schema`。

**不要和「蒸馏」搞混**：Index/Source 服务**当前工程**的活索引；离线文档包是 `gasp_kb` / `{工程}_kb`。专页：[32-index-and-source.md](../.Knowledges/32-index-and-source.md)。

## 6. 项目蒸馏（离线知识包）

目标：在工程 A 蒸馏一次，得到可拷贝的 `Axon{工程}KB` 插件；在工程 B 启用该插件后，Agent 仍可查询工程 A 的蒸馏文档（无需工程 A 的 Content）。

### 你怎么用

1. 确认已启用 **Axon** + **AxonKnowledgeLib**。  
2. 对 Agent 说：**「蒸馏本项目」**（或 `distill this project`）。  
3. Agent 应自动：
   - 从工程名推导 `Axon{Project}KB` / `{snake}_kb`
   - 调用 `knowledge_query` → `scaffold_kb_plugin` 写盘
   - 关编辑器 → 编译 → 重启
   - `extract_*` 把证据写入该插件 `Knowledge/_raw/`
   - 撰写 `Knowledge/*.md` 并用 `{ns}_query` 验收  
4. 需要带到其他工程时：拷贝整个 `Plugins/AxonMCPs/AxonXxxKB/`（并确保目标工程有 AxonKnowledgeLib），在 `.uproject` 启用即可。

### 边界（请预期）

- 新插件必须编译重启后，新的 `{ns}_query` 才会出现。  
- Markdown 全书由 Agent 蒸馏，不是 C++ 一键生成。  
- 示例包：本仓库的 **AxonGaspKB**（`gasp_kb_query`）已是蒸馏成品，不要为 GASP 再 scaffold 一套新名字。

AI 强制步骤见 [axon_guide.md](axon_guide.md) 的 **Distill current project**；细节见 [31-knowledge-distill.md](../.Knowledges/31-knowledge-distill.md)。

## 7. 扩展开发者（加自己的工具）

不要改 AxonCore 塞业务。在 `Plugins/AxonMCPs/` 下建 sibling Editor 插件，依赖 Axon，在 `StartupModule` 里 `FAxonToolRegistry::RegisterAction`。

- 门禁流程与脚手架：`axon_guide` / [axon_guide.md](axon_guide.md)  
- 手工步骤：[SIBLING_PLUGIN_GUIDE.md](SIBLING_PLUGIN_GUIDE.md)  
- Checklist：知识库 [50-extension-cookbook.md](../.Knowledges/50-extension-cookbook.md)  
- **知识包**请用 `knowledge.scaffold_kb_plugin`，不要用通用 `axon_create_extension`

## 8. 常见问题

| 现象 | 处理 |
|------|------|
| Connection refused | 编辑器未开、插件未启用、端口不一致、MCP 被关掉 |
| 找不到某个 namespace | 对应 sibling 未启用；`axon_discover` 核对 |
| PIE 弹编译错误对话框后 Agent 卡住 | 先用 `list_errored_blueprints`；或 smoke/timeseries 的 `on_compile_errors`（默认 `refuse`） |
| Load 地图报 World Memory Leaks | 先 `stop_pie` / `stop_pie_smoke`，再 load |
| bulk_fill 失败 | 该 namespace 是否注册了 adapter（目前常见：animation / blueprint / gas）；先 `dry_run=true` |
| 连上了但工具不像 Axon | 检查是否误连 Monolith 9316 |
| `source` 查不到符号 / 空索引 | 先 `source_query` → `trigger_reindex`（首次全量，耗时） |
| `project` 结果过旧 | `refresh_assets`；或等启动增量完成后再搜 |
| 说了「蒸馏」但没出新插件 | 确认 AxonKnowledgeLib 已启用；Agent 须走 `scaffold_kb_plugin` 而非只 dump 资产 |
| 找不到 `knowledge` / 新 `{ns}_kb` | 插件未编译或未重启；`axon_discover` 核对 |
| 想复用 GASP 知识 | 启用 AxonGaspKB，用 `gasp_kb_query`（search/read） |

## 9. 相关文档

| 文档 | 用途 |
|------|------|
| [3C_WORKFLOWS.md](3C_WORKFLOWS.md) | 角色 / 镜头 / 操控推荐步骤 |
| [SPEC_CORE.md](SPEC_CORE.md) | Core 规格摘要 |
| [axon_guide.md](axon_guide.md) | Agent 英文 recipes（含 Distill） |
| [SIBLING_PLUGIN_GUIDE.md](SIBLING_PLUGIN_GUIDE.md) | 扩展插件接入 |
| [../.Knowledges/32-index-and-source.md](../.Knowledges/32-index-and-source.md) | Index / Source 专页 |
| [../.Knowledges/31-knowledge-distill.md](../.Knowledges/31-knowledge-distill.md) | 项目蒸馏专页 |
| [../.Knowledges/00-routing.md](../.Knowledges/00-routing.md) | AI 文档路由 |
