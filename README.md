# NeptuneEqicMcp

MCP 服务器，用于通过 Claude 操作 Unreal Engine 编辑器。

## 架构

```
Claude Code → MCP Server (TypeScript) → WebSocket → Unreal Plugin (C++)
```

## 安装

### 1. 安装 MCP 服务器

```bash
git clone https://github.com/ogurishun2026/NeptuneEqicMcp.git
cd NeptuneEqicMcp
npm install
npm run build
```

### 2. 安装 Unreal 插件

1. 将 `unreal-plugin` 目录复制到你的 Unreal 项目的 `Plugins` 目录
   - 源路径: `NeptuneEqicMcp/unreal-plugin/`
   - 目标路径: `你的项目/Plugins/NeptuneEqicMcp/`
2. 在 Unreal 编辑器中启用 "NeptuneEqicMcp" 插件
3. 重启 Unreal 编辑器

### 3. 验证插件运行

1. 打开 Unreal 编辑器
2. 打开菜单: Edit → Plugins
3. 搜索 "NeptuneEqicMcp"，确认已启用
4. 打开: Window → Developer Tools → Output Log
5. 查找日志: `WebSocket server started on port 18765`

### 4. 配置 Claude Code MCP

在 Claude Code 项目目录创建 `.claude/settings.json`：

```json
{
  "mcpServers": {
    "neptune-eqic": {
      "command": "node",
      "args": ["你的路径/NeptuneEqicMcp/dist/index.js"],
      "env": {
        "UNREAL_HOST": "localhost",
        "UNREAL_PORT": "18765"
      }
    }
  }
}
```

## 可用工具

| 工具 | 描述 |
|------|------|
| `actor_create` | 创建 Actor |
| `actor_delete` | 删除 Actor |
| `actor_find` | 查找 Actor |
| `actor_list` | 列出所有 Actor |
| `actor_get_transform` | 获取 Transform |
| `actor_set_transform` | 设置 Transform |
| `actor_get_property` | 获取属性 |
| `actor_set_property` | 设置属性 |
| `actor_add_component` | 添加组件 |
| `actor_remove_component` | 移除组件 |
| `actor_get_components` | 获取组件列表 |

## 使用示例

```
用户: 列出当前场景中的所有 Actor
Claude: 调用 actor_list

用户: 创建一个叫 TestCube 的 Cube
Claude: 调用 actor_create

用户: 把它移动到位置 (100, 200, 300)
Claude: 调用 actor_set_transform
```

## 端口说明

| 端口 | 用途 |
|------|------|
| 18765 | WebSocket 服务器（Unreal 插件） |

## 故障排除

### 插件编译失败
- 确保 UE 5.6+ 已安装
- 删除 `Binaries` 和 `Intermediate` 目录后重新编译

### WebSocket 服务器未启动
- 检查 Output Log 是否有错误
- 确认端口 18765 未被占用

### MCP 连接失败
- 确认 Unreal 编辑器已启动
- 确认插件已加载
- 检查防火墙设置

## 版本支持

- Unreal Engine 5.6
- Unreal Engine 5.7

## 许可证

MIT
