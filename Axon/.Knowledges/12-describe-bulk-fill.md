# describe / bulk_fill / dry_run

> **角色**：说明反射描述与批量写入框架，以及 adapter 扩展点。  
> **何时阅读**：改 ReflectionWalker、DryRun、或为 sibling 注册 adapter 时。  
> **相关源码**：`Source/AxonCore/Private/Actions/AxonBulkFillActions.cpp`、`Public/AxonBulkFillRegistry.h`、`Public/Reflection/*`、各 `*BulkFillAdapter.*`  
> **相关文档**：[01-architecture.md](01-architecture.md)、[50-extension-cookbook.md](50-extension-cookbook.md)  
> **最后更新**：2026-08-01

## 保留 namespace

| Action | 用途 |
|--------|------|
| `bulk_fill.apply` | JSON tree → 反射写入；`dry_run` / `strict` |
| `bulk_fill.list_namespaces` | 已注册 adapter |
| `describe.schema` | 目标 schema 树 |
| `describe.list_targets` | 可 introspect 目标 |
| `describe.action_schema` | 任意已注册 Action 的 ParamSchema |

MCP：`bulk_fill_query` / `describe_query`，`arguments.action` 取上表短名。

## 类型链（概念）

`FAxonBulkFillSpec` → namespace adapter → `FAxonReflectionWalker` → `FAxonDryRunReport` / `FAxonDryRunGuard`

## 已注册 adapter（源码确认）

| Namespace | 位置 |
|-----------|------|
| `animation` | `AxonAnimation` → `AxonAnimationBulkFillAdapter` |
| `blueprint` | `AxonBlueprint` → `AxonBlueprintBulkFillAdapter` |
| `gas` | `AxonGAS` → `AxonGASBulkFillAdapter` |

无 adapter 的 namespace 不能 `bulk_fill.apply`；仍可用 `describe.action_schema` 查 Action 参数。

## 推荐用法

1. `describe_query` 了解形状。  
2. `bulk_fill_query` + `dry_run=true` 看报告。  
3. 确认后再 `dry_run=false` 写入。  
4. 需要时配合 `editor.save_packages`（注意 dirty 范围）。

## 待充实

- `strict` 模式精确语义与错误条目字段表。
- SoftObject / Class 路径 `_C` 规范化规则全集。
