---
name: 开工
description: 拉取 GitHub 最新代码，开始工作
---

# 开工

当用户说"开工"、"开始工作"、"拉取代码"时执行。

## 执行步骤

1. 检查当前是否在项目目录 `C:\Users\MSI-NB\Desktop\NeptuneEqicMcp_fresh`
2. 执行 `git pull` 拉取最新代码
3. 如果有更新，显示更新的内容摘要
4. 检查是否需要重新构建 (`npm run build`)

## 命令

```bash
cd "C:/Users/MSI-NB/Desktop/NeptuneEqicMcp_fresh"
git pull
npm run build
```

## 输出

告诉用户：
- 是否有更新
- 更新了什么内容
- 是否需要重新构建
