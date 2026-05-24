#!/bin/bash
# 启动AI服务器

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

echo "启动AI服务器..."
python3 ai_server.py &
echo "AI服务器已在后台启动，端口: 9999"
echo "日志输出到 /tmp/ai_server.log"

# 等待服务器启动
sleep 2

# 检查是否成功
if lsof -i :9999 > /dev/null 2>&1; then
    echo "AI服务器启动成功！"
else
    echo "AI服务器启动失败，请检查日志"
fi
