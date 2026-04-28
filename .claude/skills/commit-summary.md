---
name: 总结提交
description: 总结本次工作内容，写入版本更新说明并提交到 GitHub
---

# 总结提交

当用户说"总结一下"、"提交"、"写更新说明"时执行。

## 执行步骤

### 1. 收集本次更改

检查 git 状态和最近的提交：

```bash
cd "C:/Users/MSI-NB/Desktop/NeptuneEqicMcp_fresh"
git status
git log --oneline -10
git diff HEAD~1 --stat
```

### 2. 写入版本更新说明

在项目根目录创建或更新 `版本更新说明.md` 文件。

格式如下：

```markdown
# 版本更新说明

## [版本号] - 日期

### 新增
- 新功能描述

### 修改
- 修改内容描述

### 修复
- 修复问题描述

### 其他
- 其他更改

---
```

### 3. 提交并推送

```bash
git add 版本更新说明.md
git commit -m "docs: 更新版本说明"
git push
```

## 注意事项

- 版本号根据 package.json 或上次提交推断
- 日期格式：YYYY-MM-DD
- 分类清晰，简洁明了
- 如果文件已存在，追加新版本内容到文件开头
