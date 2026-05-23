# Mini-KVM

一个基于 C++17 / Qt5 的 Windows 桌面应用，用于通过 UVC 视频采集卡 + CH9329 串口 HID 芯片，实现对本机的 KVM（键盘、视频、鼠标）远程控制。功能类似 Openterface Mini-KVM，但完全自建协议栈与 UI。

---

## 主要功能

- **实时视频采集**：通过 UVC 采集卡获取目标机器画面，支持 YUYV、UYVY、YUV420P、MJPEG 等格式的软件解码与硬件加速渲染。
- **鼠标转发**：绝对坐标模式，将本机鼠标移动、点击、滚轮实时映射到目标机器（CH9329 绝对鼠标 HID 报告，4096×4096 分辨率）。
- **键盘转发**：支持字母、数字、功能键、符号键、导航键及修饰键（Ctrl / Shift / Alt / Win）的 HID 键盘报告转发。
- **双击优化**：针对 Qt 双击事件序列与驱动重复 Release 问题做了专门修复，保证目标机双击正常触发。
- **截图**：支持保存当前视频帧为 PNG / JPEG，也支持通过内置 HTTP Server 对外提供 `GET /capture` 接口。
- **全屏模式**：快捷键 `Ctrl+M` 切换，自动隐藏菜单栏/工具栏/状态栏。
- **串口配置**：支持 9600 / 57600 / 115200 波特率选择，连接失败自动 fallback 到 115200。

---

## 平台要求

| 项目 | 要求 |
|------|------|
| 操作系统 | Windows 10 / 11 (x64) |
| 编译器 | clang-cl（MSVC-like 前端） |
| 构建工具 | CMake >= 3.20, Ninja |
| 包管理器 | vcpkg（`x64-windows-static` triplet） |

---

## 核心依赖

| 依赖 | 版本 | 用途 |
|------|------|------|
| Qt5 | 5.x | GUI 框架（Widgets、Multimedia、MultimediaWidgets、SerialPort） |
| cpp-httplib | 单头文件版 | 内嵌 HTTP Server，提供截图 API |
| Windows SDK | 10.x | rc.exe、mt.exe、WMF 后端 |

vcpkg 安装命令参考：

```powershell
vcpkg install qt5-base qt5-multimedia qt5-serialport --triplet x64-windows-static
```

---

## 硬件准备

1. **UVC 视频采集卡**：USB 插入本机，在设备管理器中识别为摄像头。
2. **CH9329 模块**：通过 USB 转串口芯片（如 CH340）连接到本机，在设备管理器中识别为 COM 口。
3. 将 CH9329 的 USB 输出端插入**目标机器**，目标机即可接收到转发过来的键鼠信号。

---

## 项目结构

```
src/
├── model/           # 数据层：设备抽象、协议编码、硬件通信
│   ├── VideoCaptureModel    # UVC 枚举、启停、帧抓取、YUV→RGB 解码
│   ├── SerialPortManager    # 串口枚举、打开/关闭、收发
│   ├── MouseModel           # 鼠标 HID 报告、绝对坐标映射
│   ├── KeyboardModel        # 键盘 HID 报告、Qt 键值→HID 码映射
│   └── CH9329Protocol       # CH9329 帧构造、校验和
├── view/            # 表现层：纯 UI，无业务逻辑
│   ├── MainWindow           # 主窗口、工具栏、菜单、状态栏
│   ├── VideoWidget          # 视频帧渲染（30fps 节流、居中缩放）
│   └── HttpConfigDialog     # HTTP Server 端口配置对话框
├── controller/      # 控制层：协调 Model 与 View，处理输入
│   ├── DeviceCoordinator    # 设备生命周期与信号路由
│   ├── InputController      # 鼠标/键盘事件拦截、节流、转发
│   └── HttpServerController # 内嵌 HTTP Server（cpp-httplib）
└── main.cpp

docs/
├── handbook.md      # 开发手册（协议详解、问题排查）
└── gui.md           # GUI 布局与信号说明

scripts/
└── capture.py       # Python 截图客户端示例（调用 HTTP API）
```

---

## 编译

### 方式一：使用 build.bat（推荐）

确保 `clang-cl` 和 Windows SDK 的 `rc.exe` 已在 PATH 中，或能被脚本自动找到：

```bat
build.bat
```

脚本会自动完成 CMake 配置 + 编译 + 运行。

### 方式二：手动 CMake

```bat
# 配置（Release + 静态 Qt5）
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=clang-cl -DCMAKE_TOOLCHAIN_FILE=C:/src/vcpkg/scripts/buildsystems/vcpkg.cmake

# 编译
cmake --build build --parallel

# 产物
# build/mini_kvm.exe
```

> **注意**：`CMakeLists.txt` 中默认 vcpkg 路径为 `C:/src/vcpkg`，如果你的路径不同，请修改或传入 `-DVCPKG_ROOT=你的路径`。

---

## 运行

直接执行编译产物：

```bat
build\mini_kvm.exe
```

Release 版本使用 `/SUBSYSTEM:WINDOWS`，无控制台窗口；Debug 版本保留控制台，方便查看串口收发日志。

由于采用 **静态链接 Qt5**，运行时不依赖任何 Qt DLL，单文件即可部署。

---

## 快速使用

1. 启动程序，在工具栏**视频源**下拉框中选择 UVC 采集卡。
2. 在**设置**下拉框中选择需要的分辨率/格式/帧率，点击 **Connect**。
3. 在**串口**下拉框中选择 CH9329 对应的 COM 口，选择波特率，点击 **Open**。
4. 将鼠标移入中央视频区，即可控制目标机器；键盘焦点在视频区时，按键会转发到目标机。
5. 勾选**隐藏本机鼠标**，可避免本地光标遮挡画面。

### HTTP 截图 API

在 **配置(C) → HTTP配置** 中启用 HTTP Server（默认端口 8765），然后：

```bash
# 浏览器或 curl
curl http://127.0.0.1:8765/capture -o screenshot.png

# 或使用附带的 Python 脚本
python scripts/capture.py
```

---

## 技术亮点

- **高性能 YUV 解码**：使用整数快速算法 + 原始指针批量写入，避免逐像素浮点转换。
- **零拷贝安全**：对 Qt 零拷贝 RGB 格式做深拷贝后再跨线程传递，防止 `unmap()` 后悬空指针。
- **输入延迟优化**：鼠标移动 10ms 软件节流（100Hz），按钮点击/滚轮强制立即发送，兼顾流畅与响应。
- **双击防抖**：锁定 500ms / 30px 内的双击坐标，拦截驱动重复 Release，修复 Qt 双击事件序列兼容性问题。
- **线程安全 HTTP Server**：server 线程独立编码 PNG，主线程通过 `QMutex` 交换帧数据，避免 `BlockingQueuedConnection` 死锁。

---

## 相关文档

- [开发手册（协议详解、问题排查）](docs/handbook.md)
- [GUI 布局与信号说明](docs/gui.md)
