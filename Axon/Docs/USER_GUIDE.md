# Axon 用户与开发者指南

Axon 是一套跑在 **Unreal Editor** 里的 MCP 服务，面向游戏 **3C（角色 / 镜头 / 操控）** 的制作与验证：让 Cursor 等 Agent 通过工具调用完成脚手架、PIE 抽检、动画/GAS/蓝图 authoring，而不必人工点遍编辑器。

更细的内部架构与 AI 协作约定见插件知识库：[`.Knowledges/`](../.Knowledges/)（入口 [`../README.md`](../README.md)）。3C 操作链见 [3C_WORKFLOWS.md](3C_WORKFLOWS.md)。

## 1. 你需要什么

- 启用插件：**Axon**（必选），以及按需的 sibling：
  - **AxonAnimation** — 动画 / Chooser / PoseSearch
  - **AxonBlueprint** — 蓝图与 Motion Matching 脚手架
  - **AxonGAS** — Gameplay Abilities
  - **AxonConfig** — 配置查询
  - **AxonRewindDebugger** — 回放采样（镜头相关需项目里的 CameraBlueprint）
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

## 5. 扩展开发者（加自己的工具）

不要改 AxonCore 塞业务。在 `Plugins/AxonMCPs/` 下建 sibling Editor 插件，依赖 Axon，在 `StartupModule` 里 `FAxonToolRegistry::RegisterAction`。

- 门禁流程与脚手架：`axon_guide` / [axon_guide.md](axon_guide.md)  
- 手工步骤：[SIBLING_PLUGIN_GUIDE.md](SIBLING_PLUGIN_GUIDE.md)  
- Checklist：知识库 [50-extension-cookbook.md](../.Knowledges/50-extension-cookbook.md)

## 6. 常见问题

| 现象 | 处理 |
|------|------|
| Connection refused | 编辑器未开、插件未启用、端口不一致、MCP 被关掉 |
| 找不到某个 namespace | 对应 sibling 未启用；`axon_discover` 核对 |
| PIE 弹编译错误对话框后 Agent 卡住 | 先用 `list_errored_blueprints`；或 smoke/timeseries 的 `on_compile_errors`（默认 `refuse`） |
| Load 地图报 World Memory Leaks | 先 `stop_pie` / `stop_pie_smoke`，再 load |
| bulk_fill 失败 | 该 namespace 是否注册了 adapter（目前常见：animation / blueprint / gas）；先 `dry_run=true` |
| 连上了但工具不像 Axon | 检查是否误连 Monolith 9316 |

## 7. 相关文档

| 文档 | 用途 |
|------|------|
| [3C_WORKFLOWS.md](3C_WORKFLOWS.md) | 角色 / 镜头 / 操控推荐步骤 |
| [SPEC_CORE.md](SPEC_CORE.md) | Core 规格摘要 |
| [axon_guide.md](axon_guide.md) | Agent 英文 recipes |
| [SIBLING_PLUGIN_GUIDE.md](SIBLING_PLUGIN_GUIDE.md) | 扩展插件接入 |
| [../.Knowledges/00-routing.md](../.Knowledges/00-routing.md) | AI 文档路由 |
