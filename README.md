# Unreal MCP Server

MCP服务器，用于通过Claude操作Unreal Engine编辑器。

## 架构

Claude -> MCP Server (TypeScript) -> WebSocket -> Unreal Plugin (C++)

## 快速开始

### 1. 安装MCP服务器

```bash
cd unreal-mcp-server
npm install
npm run build
```

### 2. 安装Unreal插件

1. 将 `unreal-plugin` 目录复制到你的Unreal项目的 `Plugins` 目录
2. 在Unreal编辑器中启用 "Unreal MCP" 插件
3. 重启Unreal编辑器

### 3. 配置MCP

在Claude Code中添加MCP服务器配置：

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

## 可用工具

| 工具 | 描述 |
|------|------|
| `actor_create` | 创建Actor |
| `actor_delete` | 删除Actor |
| `actor_find` | 查找Actor |
| `actor_list` | 列出所有Actor |
| `actor_get_transform` | 获取Transform |
| `actor_set_transform` | 设置Transform |
| `actor_get_property` | 获取属性 |
| `actor_set_property` | 设置属性 |
| `actor_add_component` | 添加组件 |
| `actor_remove_component` | 移除组件 |
| `actor_get_components` | 获取组件列表 |

## 版本支持

- Unreal Engine 5.6
- Unreal Engine 5.7
