# Unreal MCP Server 项目总结

## 项目概述

构建一个MCP服务器，使Claude能够通过自然语言操作Unreal Engine编辑器。采用插件模式，MCP服务器作为中间层转发命令到Unreal内置插件执行。

## 技术选型

| 项目 | 选择 | 说明 |
|------|------|------|
| MCP服务器语言 | TypeScript/Node.js | 官方SDK支持完善，类型安全 |
| 通信协议 | WebSocket | 双向通信，支持事件推送 |
| 数据格式 | JSON | 简单直观，Actor操作够用 |
| 连接模式 | 单连接持久化 | 支持实时事件推送 |
| 错误处理 | 结构化错误 | 错误码+类型+信息+堆栈 |
| 引擎版本 | UE 5.6 ~ 5.7 | 简化版本兼容问题 |

## 架构设计

```
┌─────────────────────────────────────────────────────────────┐
│                         Claude                              │
└─────────────────────────┬───────────────────────────────────┘
                          │ MCP Protocol
                          ▼
┌─────────────────────────────────────────────────────────────┐
│                   MCP Server (TypeScript)                   │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────┐  │
│  │ Connection  │  │  Registry   │  │  EventDispatcher    │  │
│  │  Manager    │  │             │  │                     │  │
│  └──────┬──────┘  └──────┬──────┘  └──────────┬──────────┘  │
│         │                │                    │              │
│  ┌──────▼────────────────▼────────────────────▼──────────┐  │
│  │                    Tools (可插拔)                       │  │
│  │  actor | blueprint | animation | data | plugin | ...   │  │
│  └─────────────────────────────────────────────────────────┘  │
└─────────────────────────┬───────────────────────────────────┘
                          │ WebSocket (JSON)
                          ▼
┌─────────────────────────────────────────────────────────────┐
│                 Unreal Plugin (C++)                         │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────┐  │
│  │  WebSocket  │  │  Command    │  │   EventEmitter      │  │
│  │   Server    │  │ Dispatcher  │  │                     │  │
│  └──────┬──────┘  └──────┬──────┘  └──────────┬──────────┘  │
│         │                │                    │              │
│  ┌──────▼────────────────▼────────────────────▼──────────┐  │
│  │                   Handlers (可扩展)                     │  │
│  │  ActorHandler | BlueprintHandler | AnimationHandler    │  │
│  └─────────────────────────────────────────────────────────┘  │
└─────────────────────────┬───────────────────────────────────┘
                          │ Unreal API
                          ▼
┌─────────────────────────────────────────────────────────────┐
│                    Unreal Engine                            │
└─────────────────────────────────────────────────────────────┘
```

## 通信协议

### 命令格式（MCP → Unreal）

```json
{
  "id": "uuid-1234",
  "cmd": "actor.create",
  "params": {
    "name": "MyCube",
    "location": [0, 0, 0],
    "rotation": [0, 0, 0],
    "scale": [1, 1, 1]
  }
}
```

### 响应格式（Unreal → MCP）

**成功：**
```json
{
  "id": "uuid-1234",
  "success": true,
  "data": {
    "actorId": "abc123",
    "name": "MyCube"
  }
}
```

**失败：**
```json
{
  "id": "uuid-1234",
  "success": false,
  "error": {
    "code": "ACTOR_SPAWN_FAILED",
    "type": "runtime_error",
    "message": "Failed to spawn actor: invalid location",
    "stack": "ActorHandler.cpp:45"
  }
}
```

### 事件格式（Unreal → MCP）

```json
{
  "event": "actor.deleted",
  "timestamp": "2026-04-27T10:30:00Z",
  "data": {
    "actorId": "abc123",
    "name": "MyCube"
  }
}
```

## 项目结构

```
D:/unreal-mcp-server/
├── src/
│   ├── index.ts              # 入口文件
│   ├── core/
│   │   ├── connection.ts     # WebSocket连接管理
│   │   ├── protocol.ts       # MCP协议处理
│   │   ├── eventDispatcher.ts # 事件分发
│   │   └── __tests__/        # 测试文件
│   ├── tools/
│   │   ├── registry.ts       # 工具注册中心
│   │   └── actor/            # Actor工具模块
│   │       ├── index.ts
│   │       ├── create.ts
│   │       ├── delete.ts
│   │       ├── find.ts
│   │       ├── list.ts
│   │       ├── transform.ts
│   │       ├── property.ts
│   │       └── component.ts
│   └── types/
│       ├── commands.ts       # 命令类型定义
│       ├── events.ts         # 事件类型定义
│       └── errors.ts         # 错误类型定义
├── unreal-plugin/
│   ├── UnrealMCP.uplugin
│   └── Source/UnrealMCP/
│       ├── Public/
│       │   ├── UnrealMCP.h
│       │   ├── WebSocketServer.h
│       │   ├── CommandDispatcher.h
│       │   └── Handlers/
│       │       ├── IHandler.h
│       │       └── ActorHandler.h
│       └── Private/
│           ├── UnrealMCP.cpp
│           ├── WebSocketServer.cpp
│           ├── CommandDispatcher.cpp
│           └── Handlers/
│               └── ActorHandler.cpp
├── docs/
│   └── superpowers/
│       ├── specs/            # 设计文档
│       └── plans/            # 实现计划
├── package.json
├── tsconfig.json
└── README.md
```

## 已完成的工具

### Actor工具集（11个工具）

| 工具名 | 功能 | 参数 |
|--------|------|------|
| `actor_create` | 创建Actor | name, location, rotation, scale, actorClass |
| `actor_delete` | 删除Actor | actorId 或 name |
| `actor_find` | 查找Actor | byId / byName / byClass / byTag |
| `actor_list` | 列出Actor | filter(可选) |
| `actor_get_transform` | 获取Transform | actorId |
| `actor_set_transform` | 设置Transform | actorId, location, rotation, scale |
| `actor_get_property` | 获取属性 | actorId, propertyName |
| `actor_set_property` | 设置属性 | actorId, propertyName, value |
| `actor_add_component` | 添加组件 | actorId, componentType, name |
| `actor_remove_component` | 移除组件 | actorId, componentId |
| `actor_get_components` | 获取组件列表 | actorId |

## 扩展机制

### 添加新工具模块的步骤

以添加 **Animation** 模块为例：

**Step 1：MCP端添加工具模块**

```typescript
// src/tools/animation/index.ts
import { Tool } from '../registry.js';

export const animationTools: Tool[] = [
  {
    name: 'animation_play',
    description: '播放动画',
    params: { ... },
    execute: async (params, connection) => {
      return connection.sendCommand('animation.play', params);
    }
  }
];
```

**Step 2：注册到Registry**

```typescript
// src/tools/registry.ts
import { actorTools } from './actor';
import { animationTools } from './animation';  // 新增

export function registerAllTools(registry: Registry) {
  registry.register(actorTools);
  registry.register(animationTools);  // 新增
}
```

**Step 3：Unreal端添加Handler**

```cpp
// ActorHandler.h
class FAnimationHandler : public IHandler
{
public:
    virtual FString GetPrefix() const override { return "animation"; }
    virtual TSharedPtr<FJsonObject> Handle(...) override;
};
```

**Step 4：注册Handler**

```cpp
// UnrealMCP.cpp
RegisterHandler(MakeShared<FAnimationHandler>());
```

### 扩展模块规划

| 模块 | 预计工具数 | 优先级 | 说明 |
|------|-----------|--------|------|
| Actor | 11个 | P0（已完成） | Actor基础操作 |
| Blueprint | ~8个 | P1 | 蓝图创建、修改、编译 |
| Animation | ~6个 | P2 | 动画播放、绑定、混合 |
| Data | ~4个 | P3 | 数据表、配置文件操作 |
| Plugin | ~4个 | P4 | 插件管理 |

## 使用方法

### 1. 安装MCP服务器

```bash
cd D:/unreal-mcp-server
npm install
npm run build
```

### 2. 安装Unreal插件

1. 将 `unreal-plugin` 目录复制到你的Unreal项目的 `Plugins` 目录
2. 在Unreal编辑器中启用 "Unreal MCP" 插件
3. 重启Unreal编辑器

### 3. 配置Claude Code MCP

在Claude Code设置中添加：

```json
{
  "mcpServers": {
    "unreal": {
      "command": "node",
      "args": ["D:/unreal-mcp-server/dist/index.js"],
      "env": {
        "UNREAL_HOST": "localhost",
        "UNREAL_PORT": "8080"
      }
    }
  }
}
```

### 4. 使用示例

```
用户: 创建一个叫 "MyCube" 的Actor
Claude: [调用 actor_create 工具]
        已创建 Actor "MyCube"，ID: MyCube_123

用户: 把它移动到 (100, 200, 300)
Claude: [调用 actor_set_transform 工具]
        Transform 已更新

用户: 给它添加一个 PointLightComponent
Claude: [调用 actor_add_component 工具]
        已添加组件 PointLight
```

## 测试步骤

### 测试MCP服务器（无Unreal）

```bash
cd D:/unreal-mcp-server
npm test
```

### 测试完整流程

1. 打开Unreal项目（需要先安装插件）
2. 确认Output Log显示 "WebSocket server started on port 8080"
3. 配置Claude Code MCP
4. 在Claude中测试： "列出当前场景中的所有Actor"

## 版本兼容策略

### 引擎版本支持

| 引擎版本 | 插件版本 | 支持状态 |
|----------|----------|----------|
| UE 5.7 | 1.0.x | 兼容 |
| UE 5.6 | 1.0.x | 主要测试 |

### API差异处理

使用预处理宏处理版本差异：

```cpp
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 7
    // UE 5.7特有代码
#else
    // UE 5.6代码
#endif
```

## 远程仓库

GitHub: https://github.com/ogurishun2026/NeptuneEqicMcp.git

## 相关文档

- 设计文档: `docs/superpowers/specs/2026-04-27-unreal-mcp-design.md`
- 实现计划: `docs/superpowers/plans/2026-04-27-unreal-mcp-implementation.md`
