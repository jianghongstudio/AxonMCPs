# MCP 协议与工具映射

> **角色**：说明 Axon HTTP JSON-RPC 形态、端口、工具命名与客户端接入要点。  
> **何时阅读**：改 `FAxonHttpServer`、工具路由、或帮助用户连 Cursor 时。  
> **相关源码**：`Source/AxonCore/Private/AxonHttpServer.cpp`、`AxonCoreModule.cpp`、`AxonSettings.h`、`Templates/.mcp.json.example`  
> **相关文档**：[01-architecture.md](01-architecture.md)、[11-action-registry.md](11-action-registry.md)、[Docs/USER_GUIDE.md](../Docs/USER_GUIDE.md)、[Docs/SPEC_CORE.md](../Docs/SPEC_CORE.md)  
> **最后更新**：2026-08-01

## 传输

| 项 | 值 |
|----|-----|
| 默认端口 | **9320**（`UAxonSettings::ServerPort`） |
| URL | `http://localhost:9320/mcp` |
| 协议 | JSON-RPC 2.0（Streamable HTTP） |
| 常见方法 | `initialize`、`tools/list`、`tools/call`、`ping` |
| 其它路由 | health、CORS OPTIONS（细节以 `FAxonHttpServer` 为准） |

启动条件：`bMcpServerEnabled == true` 且非 commandlet。控制台可 `Axon.Restart` 重启（以源码为准）。

## MCP 工具名 ↔ 内部 Action

| MCP 工具 | 内部 |
|----------|------|
| `axon_discover` / `axon_status` / `axon_guide` / … | namespace `axon`，action = 后缀 |
| `{ns}_query` | namespace `{ns}`，`arguments.action` + 其余参数 |
| `describe_query` | namespace `describe` |
| `bulk_fill_query` | namespace `bulk_fill` |

调用域 Action 的典型形态：

```json
{
  "method": "tools/call",
  "params": {
    "name": "editor_query",
    "arguments": {
      "action": "run_pie_smoke",
      "map": "/Game/...",
      "duration": 5
    }
  }
}
```

（具体 JSON-RPC 信封字段以实现为准；Agent 侧通常由 MCP SDK 封装。）

## 设置

Project Settings → Plugins → **Axon**：

- `bMcpServerEnabled`（默认 true）
- `ServerPort`（默认 9320）

## 客户端配置

示例：`Templates/.mcp.json.example`

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

**注意**：工程根 `.mcp.json` 可能仍指向 Monolith（9316）。连 Axon 须单独配置或改 URL。

## 运行指示

- 状态栏：`SAxonStatusBarWidget`（Off / Listening / Connected）
- 哨兵：`Axon/Saved/.axon_running`（待充实字段稳定性说明）

## 待充实

- tools/list 分页与大型 action 枚举的性能策略。
- 鉴权 / 绑定地址（若后续增加）。
