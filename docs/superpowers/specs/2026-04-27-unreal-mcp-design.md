# Unreal MCP Server 设计文档

## 概述

为Unreal Engine构建MCP服务器，支持Claude通过自然语言操作Unreal编辑器。采用插件模式，MCP服务器作为中间层转发命令到Unreal内置插件执行。

## 技术选型

| 项目 | 选择 | 说明 |
|------|------|------|
| MCP服务器语言 | TypeScript/Node.js | 官方SDK支持完善，类型安全 |
| 通信协议 | WebSocket | 双向通信，支持事件推送 |
| 数据格式 | JSON | 简单直观，Actor操作够用 |
| 连接模式 | 单连接持久化 | 支持实时事件推送 |
| 错误处理 | 结构化错误 | 错误码+类型+信息+堆栈 |
| 引擎版本 | UE 5.6 ~ 5.7 | 简化版本兼容问题 |

## 整体架构

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

## 核心组件说明

### MCP Server端

#### Connection Manager
负责与Unreal插件建立和维护WebSocket连接。

职责：
- 启动时尝试连接Unreal插件
- 连接断开时自动重连
- 管理连接状态（连接中/已连接/断开）
- 发送命令前检查连接是否可用

#### Registry
工具注册中心，管理所有MCP工具的注册和发现。

职责：
- 启动时扫描并加载所有工具模块
- 提供工具列表给MCP协议层
- 根据工具名称路由到对应模块

#### EventDispatcher
接收来自Unreal的事件，并分发给感兴趣的订阅者。

职责：
- 监听WebSocket上的事件消息
- 根据事件类型分发到对应处理器
- 缓存最近事件（可选）

#### Tools
实现具体的MCP工具，每个领域一个模块。

职责：
- 定义工具的参数schema
- 构造命令发送给Unreal
- 解析Unreal返回的结果

### Unreal Plugin端

#### WebSocket Server
在Unreal内启动WebSocket服务器，监听MCP连接。

职责：
- 启动/停止WebSocket服务
- 接收MCP发来的JSON命令
- 发送事件到MCP

#### Command Dispatcher
解析收到的命令，路由到对应的Handler。

职责：
- 解析JSON命令
- 根据命令类型找到对应Handler
- 调用Handler执行命令
- 收集结果或错误，返回给MCP

#### EventEmitter
监听Unreal内部事件，推送到MCP。

职责：
- 注册Unreal事件委托
- 当事件发生时构造JSON消息
- 通过WebSocket发送给MCP

#### Handlers
执行具体的Unreal操作。

职责：
- 调用Unreal C++ API执行操作
- 返回操作结果或错误
- 每个Handler处理一个领域的命令

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

成功：
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

失败：
```json
{
  "id": "uuid-1234",
  "success": false,
  "error": {
    "code": "ACTOR_SPAWN_FAILED",
    "type": "runtime_error",
    "message": "Failed to spawn actor: invalid location",
    "stack": "ActorHandler.cpp:45\nCommandDispatcher.cpp:23"
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

### 命令命名规范

| 模块 | 命令前缀 | 示例 |
|------|----------|------|
| Actor | `actor.` | `actor.create`, `actor.delete`, `actor.find` |
| Blueprint | `blueprint.` | `blueprint.create`, `blueprint.modify` |
| Animation | `animation.` | `animation.play`, `animation.stop` |
| Data | `data.` | `data.read`, `data.write` |
| Plugin | `plugin.` | `plugin.install`, `plugin.enable` |

## 项目目录结构

### MCP Server

```
D:/unreal-mcp-server/
├── src/
│   ├── index.ts              # 入口文件
│   ├── core/
│   │   ├── connection.ts     # WebSocket连接管理
│   │   ├── protocol.ts       # MCP协议处理
│   │   └── eventDispatcher.ts # 事件分发
│   ├── tools/
│   │   ├── registry.ts       # 工具注册中心
│   │   ├── actor/            # Actor工具模块
│   │   │   ├── index.ts
│   │   │   ├── create.ts
│   │   │   ├── delete.ts
│   │   │   ├── find.ts
│   │   │   ├── transform.ts
│   │   │   ├── property.ts
│   │   │   └── component.ts
│   │   ├── blueprint/        # 蓝图模块（后续扩展）
│   │   ├── animation/        # 动画模块（后续扩展）
│   │   ├── data/             # 数据模块（后续扩展）
│   │   └── plugin/           # 插件模块（后续扩展）
│   └── types/
│       ├── commands.ts       # 命令类型定义
│       ├── events.ts         # 事件类型定义
│       └── errors.ts         # 错误类型定义
├── package.json
├── tsconfig.json
└── README.md
```

### Unreal Plugin

```
D:/unreal-mcp-server/unreal-plugin/
├── Source/
│   ├── UnrealMCP/
│   │   ├── Private/
│   │   │   ├── UnrealMCP.cpp
│   │   │   ├── WebSocketServer.cpp
│   │   │   ├── CommandDispatcher.cpp
│   │   │   ├── EventEmitter.cpp
│   │   │   └── Handlers/
│   │   │       ├── ActorHandler.cpp
│   │   │       └── IHandler.cpp
│   │   └── Public/
│   │       ├── UnrealMCP.h
│   │       ├── WebSocketServer.h
│   │       ├── CommandDispatcher.h
│   │       ├── EventEmitter.h
│   │       └── Handlers/
│   │           ├── ActorHandler.h
│   │           └── IHandler.h
│   └── UnrealMCP.Build.cs
├── UnrealMCP.uplugin
└── Resources/
    └── Icon128.png
```

## 版本兼容策略

### 引擎版本支持

| 引擎版本 | 插件版本 | 支持状态 |
|----------|----------|----------|
| UE 5.7 | 1.0.x | 兼容 |
| UE 5.6 | 1.0.x | 主要测试 |

### 插件版本号规则

```
主版本.次版本.修订版本
  │      │      │
  │      │      └── Bug修复、小改动
  │      └── 新功能、UE小版本适配
  └── 大版本重构、不兼容变更、UE大版本升级
```

### 插件清单（.uplugin）

```json
{
    "FileVersion": 3,
    "Version": 1,
    "VersionName": "1.0.0",
    "FriendlyName": "Unreal MCP",
    "EngineVersion": "5.6.0",
    "EngineVersionMax": "5.7.*",
    "Description": "MCP server integration for Unreal Engine",
    "Modules": [
        {
            "Name": "UnrealMCP",
            "Type": "Runtime",
            "LoadingPhase": "Default"
        }
    ]
}
```

### API差异处理

使用预处理宏处理版本差异：

```cpp
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 7
    // UE 5.7特有代码
#else
    // UE 5.6代码
#endif
```

## 第一阶段：Actor工具集

### 工具列表

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

### 工具参数详细定义

**actor_create：**
```typescript
{
  name: string;                         // Actor名称
  location?: [number, number, number];  // 可选，默认[0,0,0]
  rotation?: [number, number, number];  // 可选，默认[0,0,0]
  scale?: [number, number, number];     // 可选，默认[1,1,1]
  actorClass?: string;                  // 可选，默认"Actor"
}
```

**actor_find：**
```typescript
{
  byId?: string;      // 按ID查找
  byName?: string;    // 按名称查找（支持通配符*）
  byClass?: string;   // 按类名查找
  byTag?: string;     // 按标签查找
}
```

**actor_set_property：**
```typescript
{
  actorId: string;
  propertyName: string;
  propertyType: "int" | "float" | "string" | "bool" | "vector" | "rotator" | "color";
  value: any;
}
```

## 扩展机制

### 添加新工具模块的步骤

以添加 **Animation** 模块为例：

**Step 1：MCP端添加工具模块**

```typescript
// src/tools/animation/index.ts
import { Tool } from '../registry';

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
// Source/UnrealMCP/Private/Handlers/AnimationHandler.h
class FAnimationHandler : public IHandler
{
public:
    virtual FString GetPrefix() const override { return "animation"; }
    virtual TSharedPtr<FJsonObject> Handle(
        const FString& Command,
        const FJsonObject& Params
    ) override;
};
```

**Step 4：注册Handler**

```cpp
// CommandDispatcher.cpp
void FCommandDispatcher::RegisterHandlers()
{
    RegisterHandler(MakeShared<FActorHandler>());
    RegisterHandler(MakeShared<FAnimationHandler>());  // 新增
}
```

### 扩展模块规划

| 模块 | 预计工具数 | 优先级 |
|------|-----------|--------|
| Actor | 11个 | P0（当前） |
| Blueprint | ~8个 | P1 |
| Animation | ~6个 | P2 |
| Data | ~4个 | P3 |
| Plugin | ~4个 | P4 |

## 交付计划

### 第一阶段
- MCP服务器核心框架（connection, protocol, registry, eventDispatcher）
- Unreal插件核心框架（WebSocketServer, CommandDispatcher, EventEmitter）
- Actor工具集（11个工具）
- 事件推送基础

### 后续阶段
- P1: Blueprint模块
- P2: Animation模块
- P3: Data模块
- P4: Plugin模块
