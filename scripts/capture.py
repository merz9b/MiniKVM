#!/usr/bin/env python3
"""Mini-KVM HTTP 截图客户端

用法:
    python capture.py           # 默认保存为 capture.png
    python capture.py shot.png  # 指定文件名
"""

import sys
import requests

URL = "http://127.0.0.1:8765/capture"
DEFAULT_NAME = "capture.png"


def main():
    output = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_NAME

    try:
        resp = requests.get(URL, timeout=5)
        resp.raise_for_status()
    except requests.exceptions.ConnectionError:
        print(f"[ERROR] 无法连接到 {URL}")
        print("        请确认 Mini-KVM 已启动且 HTTP Server 已启用")
        sys.exit(1)
    except requests.exceptions.Timeout:
        print(f"[ERROR] 请求超时")
        sys.exit(1)

    with open(output, "wb") as f:
        f.write(resp.content)

    print(f"[OK] 截图已保存: {output} ({len(resp.content)} bytes)")


if __name__ == "__main__":
    main()
