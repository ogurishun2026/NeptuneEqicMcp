#!/bin/bash
# NeptuneEqicMcp 云端部署脚本
# 在服务器上执行: curl -fsSL https://raw.githubusercontent.com/ogurishun2026/NeptuneEqicMcp/main/deploy-cloud.sh | bash

set -e

echo "=== NeptuneEqicMcp 云端部署 ==="

# 检查 Node.js
if ! command -v node &> /dev/null; then
    echo "安装 Node.js 18..."
    curl -fsSL https://deb.nodesource.com/setup_18.x | sudo -E bash -
    sudo apt-get install -y nodejs
fi

echo "Node.js 版本: $(node -v)"

# 安装 pm2
if ! command -v pm2 &> /dev/null; then
    echo "安装 pm2..."
    sudo npm install -g pm2
fi

# 克隆或更新仓库
if [ -d "NeptuneEqicMcp" ]; then
    echo "更新仓库..."
    cd NeptuneEqicMcp
    git pull
else
    echo "克隆仓库..."
    git clone https://github.com/ogurishun2026/NeptuneEqicMcp.git
    cd NeptuneEqicMcp
fi

# 安装依赖并构建
echo "安装依赖..."
npm install

echo "构建项目..."
npm run build

# 创建云端配置
echo "创建云端配置..."
cat > mcp-config.json << 'EOF'
{
  "mode": "cloud",
  "local": {
    "host": "localhost",
    "port": 18765
  },
  "cloud": {
    "host": "0.0.0.0",
    "port": 18765
  }
}
EOF

# 开放防火墙端口
echo "配置防火墙..."
sudo ufw allow 18765/tcp 2>/dev/null || echo "防火墙配置跳过（可能需要手动配置）"

# 启动服务
echo "启动服务..."
pm2 delete neptune-mcp 2>/dev/null || true
pm2 start dist/index.js --name neptune-mcp
pm2 save

echo ""
echo "=== 部署完成 ==="
echo "MCP Server 已启动在端口 18765"
echo ""
echo "查看状态: pm2 status"
echo "查看日志: pm2 logs neptune-mcp"
echo ""
echo "本地 Unreal 配置:"
echo "  mcp-config.json 中 mode 设为 \"cloud\""
echo "  cloud.host 设为 \"182.92.96.224\""
