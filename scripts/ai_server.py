#!/usr/bin/env python3
"""
AI Server for MUD NPC System
支持多种AI模型API（Anthropic、OpenAI兼容等）
"""

import json
import socket
import threading
import os
import sys
import urllib.request
import urllib.error
import time

# 配置文件路径
CONFIG_FILE = "/Users/tiandongtao/WorkSpace/xyj450/world/adm/etc/ai_config"

# 默认配置
DEFAULT_CONFIG = {
    "api_endpoint": "https://qianfan.baidubce.com/anthropic/coding",
    "api_key": "",
    "model": "DeepSeek-V4-Flash",
    "max_tokens": 500,
    "timeout": 30,
    "host": "127.0.0.1",
    "port": 9999
}

config = DEFAULT_CONFIG.copy()

def load_config():
    """加载配置文件"""
    global config
    if os.path.exists(CONFIG_FILE):
        try:
            with open(CONFIG_FILE, 'r', encoding='utf-8') as f:
                user_config = json.load(f)
                config.update(user_config)
                print(f"Config loaded from {CONFIG_FILE}", file=sys.stderr)
        except Exception as e:
            print(f"Error loading config: {e}, using defaults", file=sys.stderr)

    # 也支持单独的api_key文件
    key_file = "/Users/tiandongtao/WorkSpace/xyj450/world/adm/etc/ai_key"
    if os.path.exists(key_file) and not config.get("api_key"):
        with open(key_file, 'r') as f:
            config["api_key"] = f.read().strip()

    return config

def call_anthropic_api(messages, system_prompt, model, max_tokens, timeout):
    """调用OpenAI格式API"""
    api_key = config.get("api_key", "")
    endpoint = config.get("api_endpoint", "")

    headers = {
        "Content-Type": "application/json",
        "Authorization": f"Bearer {api_key}"
    }

    # 将system prompt合并到messages中（OpenAI格式）
    full_messages = [{"role": "system", "content": system_prompt}] + messages

    payload = {
        "model": model,
        "max_tokens": max_tokens,
        "messages": full_messages
    }

    data = json.dumps(payload, ensure_ascii=False).encode('utf-8')

    req = urllib.request.Request(
        endpoint,
        data=data,
        headers=headers,
        method='POST'
    )

    try:
        with urllib.request.urlopen(req, timeout=timeout) as response:
            result = json.loads(response.read().decode('utf-8'))
            return result
    except urllib.error.HTTPError as e:
        error_body = e.read().decode('utf-8')
        return {"error": f"HTTP {e.code}: {error_body}"}
    except urllib.error.URLError as e:
        return {"error": f"URL Error: {e.reason}"}
    except Exception as e:
        return {"error": str(e)}

def parse_response(response):
    """解析API响应"""
    if "error" in response:
        return response["error"]

    # Anthropic/Claude格式
    if "content" in response:
        for block in response["content"]:
            if block.get("type") == "text":
                return block.get("text", "")

    # OpenAI格式
    if "choices" in response and len(response["choices"]) > 0:
        choice = response["choices"][0]
        if "message" in choice:
            return choice["message"].get("content", "")
        if "text" in choice:
            return choice["text"]

    return "无法解析响应"

def handle_client(conn, addr):
    """处理客户端请求"""
    try:
        conn.settimeout(5.0)  # 设置5秒超时
        data = b""

        # 读取数据直到收到完整JSON或超时
        while True:
            try:
                chunk = conn.recv(4096)
                if not chunk:
                    break
                data += chunk
                # 检查是否收到完整JSON（简单检测：以}结尾）
                try:
                    json.loads(data.decode('utf-8'))
                    break  # 解析成功，说明收到完整请求
                except json.JSONDecodeError:
                    continue  # 继续读取
            except socket.timeout:
                break  # 超时，使用已收到的数据

        if not data:
            return

        request = json.loads(data.decode('utf-8'))

        messages = request.get("messages", [])
        system_prompt = request.get("system", "")
        model = request.get("model", config.get("model"))
        max_tokens = request.get("max_tokens", config.get("max_tokens"))
        timeout = request.get("timeout", config.get("timeout"))

        print(f"[{time.strftime('%H:%M:%S')}] Request from {addr}, model: {model}", file=sys.stderr)

        start_time = time.time()
        response = call_anthropic_api(messages, system_prompt, model, max_tokens, timeout)
        elapsed = time.time() - start_time

        reply_text = parse_response(response)

        result = {
            "reply": reply_text,
            "elapsed": elapsed,
            "success": "error" not in response
        }

        conn.sendall(json.dumps(result, ensure_ascii=False).encode('utf-8'))
        print(f"[{time.strftime('%H:%M:%S')}] Response sent ({elapsed:.2f}s)", file=sys.stderr)

    except Exception as e:
        print(f"Error handling client {addr}: {e}", file=sys.stderr)
        try:
            conn.sendall(json.dumps({"error": str(e), "success": False}).encode('utf-8'))
        except:
            pass
    finally:
        conn.close()

def main():
    """主函数"""
    load_config()

    api_key = config.get("api_key", "")
    if not api_key:
        print("=" * 50, file=sys.stderr)
        print("WARNING: API key not configured!", file=sys.stderr)
        print(f"Please set api_key in: {CONFIG_FILE}", file=sys.stderr)
        print("=" * 50, file=sys.stderr)

    host = config.get("host", "127.0.0.1")
    port = config.get("port", 9999)

    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind((host, port))
    server.listen(5)

    print(f"\n{'=' * 50}", file=sys.stderr)
    print(f"AI Server started", file=sys.stderr)
    print(f"Listening on {host}:{port}", file=sys.stderr)
    print(f"Model: {config.get('model', 'DeepSeek-V4-Flash')}", file=sys.stderr)
    print(f"Endpoint: {config.get('api_endpoint', '')}", file=sys.stderr)
    print(f"Press Ctrl+C to stop", file=sys.stderr)
    print(f"{'=' * 50}\n", file=sys.stderr)

    try:
        while True:
            conn, addr = server.accept()
            thread = threading.Thread(target=handle_client, args=(conn, addr))
            thread.daemon = True
            thread.start()
    except KeyboardInterrupt:
        print("\nShutting down...", file=sys.stderr)
    finally:
        server.close()

if __name__ == "__main__":
    main()
