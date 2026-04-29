# 问题：添加 SceneComponent 后无法设置 Actor 变换

**发现日期**: 2026-04-29
**问题级别**: 中等
**影响功能**: `actor.add_component`, `actor.set_transform`

## 问题描述

当使用 `actor.add_component` 给一个没有 RootComponent 的基础 `AActor` 添加 `SceneComponent` 后，调用 `actor.set_transform` 设置位置/旋转/缩放无效。

## 复现步骤

1. 创建一个基础 AActor：
```json
{
  "id": "test-1",
  "cmd": "actor.create",
  "params": {
    "class": "/Script/Engine.Actor",
    "name": "test_actor"
  }
}
```

2. 添加 SceneComponent：
```json
{
  "id": "test-2",
  "cmd": "actor.add_component",
  "params": {
    "id": "test_actor_...",
    "class": "/Script/Engine.SceneComponent",
    "name": "RootComponent"
  }
}
```

3. 尝试设置位置：
```json
{
  "id": "test-3",
  "cmd": "actor.set_transform",
  "params": {
    "id": "test_actor_...",
    "location": { "x": 6, "y": 6, "z": 6 }
  }
}
```

**预期结果**: Actor 移动到 (6, 6, 6)
**实际结果**: 位置仍为 (0, 0, 0)

## 根本原因

在 `ActorHandler.cpp` 的 `HandleAddComponent` 函数中，创建并注册了 `SceneComponent`，但没有调用 `Actor->SetRootComponent()` 将其设置为根组件。

在 UE 中，Actor 的变换操作（`SetActorLocation`, `SetActorRotation`, `SetActorScale3D`）依赖于 `RootComponent`。如果没有 RootComponent，这些调用不会生效。

## 建议修复

在 `HandleAddComponent` 中添加以下逻辑：

```cpp
// 如果是 SceneComponent 且 Actor 没有 RootComponent，设置为根组件
if (USceneComponent* SceneComp = Cast<USceneComponent>(NewComponent))
{
    if (!Actor->GetRootComponent())
    {
        Actor->SetRootComponent(SceneComp);
    }
}
```

修改位置：`unreal-plugin/Source/NeptuneEqicMcp/Private/Handlers/ActorHandler.cpp` 第 555 行附近（`NewComponent->RegisterComponent();` 之后）。

## 相关文件

- `unreal-plugin/Source/NeptuneEqicMcp/Private/Handlers/ActorHandler.cpp`
- `unreal-plugin/Source/NeptuneEqicMcp/Private/Handlers/ActorHandler.h`

## 测试环境

- UE 版本: 5.6
- 平台: Windows 11
- MCP 服务器: 本地 WebSocket 模式 (端口 18765)
