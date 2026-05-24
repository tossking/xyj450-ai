#!/usr/bin/env python3
"""
AI Client Script for MUD NPC System
使用Claude API生成NPC回复
"""

import sys
import json
import os
import time
import urllib.request
import urllib.error

API_ENDPOINT = "https://api.anthropic.com/v1/messages"
DEFAULT_MODEL = "claude-sonnet-4-6-20250514"
MAX_TOKENS = 500
TIMEOUT = 30

def call_claude_api(api_key, messages, system_prompt, model=None, max_tokens=MAX_TOKENS, timeout=TIMEOUT):
    """调用Claude API"""
    if model is None:
        model = DEFAULT_MODEL

    headers = {
        "Content-Type": "application/json",
        "x-api-key": api_key,
        "anthropic-version": "2023-06-01"
    }

    payload = {
        "model": model,
        "max_tokens": max_tokens,
        "system": system_prompt,
        "messages": messages
    }

    data = json.dumps(payload).encode('utf-8')

    req = urllib.request.Request(
        API_ENDPOINT,
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
    """解析API响应，提取文本"""
    if "error" in response:
        return f"[错误: {response['error']}]"

    if "content" in response:
        for block in response["content"]:
            if block.get("type") == "text":
                return block.get("text", "")

    return "[无法解析响应]"

def main():
    if len(sys.argv) < 4:
        print("Usage: ai_call.py <api_key> <input_file> <output_file> [timeout]")
        sys.exit(1)

    api_key = sys.argv[1]
    input_file = sys.argv[2]
    output_file = sys.argv[3]
    timeout = int(sys.argv[4]) if len(sys.argv) > 4 else TIMEOUT

    # 读取请求
    try:
        with open(input_file, 'r', encoding='utf-8') as f:
            request_data = json.load(f)
    except Exception as e:
        print(f"Error reading input: {e}", file=sys.stderr)
        sys.exit(1)

    # 提取参数
    messages = request_data.get("messages", [])
    system_prompt = request_data.get("system", "")
    model = request_data.get("model", DEFAULT_MODEL)
    max_tokens = request_data.get("max_tokens", MAX_TOKENS)

    # 调用API
    start_time = time.time()
    response = call_claude_api(api_key, messages, system_prompt, model, max_tokens, timeout)
    elapsed = time.time() - start_time

    # 解析响应
    reply_text = parse_response(response)

    # 写入输出
    result = {
        "reply": reply_text,
        "elapsed": elapsed,
        "model": model
    }

    try:
        with open(output_file, 'w', encoding='utf-8') as f:
            json.dump(result, f, ensure_ascii=False, indent=2)
    except Exception as e:
        print(f"Error writing output: {e}", file=sys.stderr)
        sys.exit(1)

    print(f"AI response generated in {elapsed:.2f}s", file=sys.stderr)

if __name__ == "__main__":
    main()
