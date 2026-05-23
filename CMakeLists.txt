cmake_minimum_required(VERSION 3.20)

# ============================================================
# vcpkg 工具链与静态 triplet
# 必须在 project() 之前设置，否则 toolchain 无法读取
# ============================================================
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(VCPKG_ROOT "C:/src/vcpkg")
    if(EXISTS "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
        set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
            CACHE FILEPATH "Vcpkg toolchain file")
    else()
        message(FATAL_ERROR "Vcpkg not found at ${VCPKG_ROOT}.")
    endif()
endif()
set(VCPKG_TARGET_TRIPLET "x64-windows-static")

project(QtTimeApp LANGUAGES CXX)

# ============================================================
# 语言标准与 Qt MOC
# ============================================================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)

# 静态链接 Qt5 需要 /MT，与 vcpkg x64-windows-static 保持一致
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")

# ============================================================
# 查找 Qt5
# ============================================================
find_package(Qt5 REQUIRED COMPONENTS Widgets Multimedia MultimediaWidgets SerialPort)

# ============================================================
# 源文件
# ============================================================
set(SOURCES
    src/main.cpp
    src/model/VideoCaptureModel.cpp
    src/model/CH9329Protocol.cpp
    src/model/SerialPortManager.cpp
    src/model/MouseModel.cpp
    src/model/KeyboardModel.cpp
    src/view/VideoWidget.cpp
    src/view/MainWindow.cpp
    src/view/HttpConfigDialog.cpp
    src/controller/DeviceCoordinator.cpp
    src/controller/InputController.cpp
    src/controller/HttpServerController.cpp
)

set(HEADERS
    src/model/VideoCaptureModel.h
    src/model/CH9329Protocol.h
    src/model/SerialPortManager.h
    src/model/MouseModel.h
    src/model/KeyboardModel.h
    src/view/VideoWidget.h
    src/view/MainWindow.h
    src/view/HttpConfigDialog.h
    src/controller/DeviceCoordinator.h
    src/controller/InputController.h
    src/controller/HttpServerController.h
)

# ============================================================
# 可执行文件
# ============================================================
add_executable(mini_kvm ${SOURCES} ${HEADERS})

# 链接 Qt5
target_link_libraries(mini_kvm PRIVATE
    Qt5::Widgets
    Qt5::Multimedia
    Qt5::MultimediaWidgets
    Qt5::SerialPort
)

# ============================================================
# 静态 Qt5 平台插件（Windows）
# ============================================================
find_library(QT_QWINDOWS_LIB qwindows
    PATHS "C:/src/vcpkg/installed/x64-windows-static/plugins/platforms"
    NO_DEFAULT_PATH
)
if(NOT QT_QWINDOWS_LIB)
    message(FATAL_ERROR "找不到 qwindows.lib，请确认安装了 qt5-base:x64-windows-static")
endif()
target_link_libraries(mini_kvm PRIVATE ${QT_QWINDOWS_LIB})

# ============================================================
# 静态 Qt5 多媒体插件（Windows）
# ============================================================
find_library(QT_WMFENGINE_LIB wmfengine
    PATHS "C:/src/vcpkg/installed/x64-windows-static/plugins/mediaservice"
    NO_DEFAULT_PATH
)
if(QT_WMFENGINE_LIB)
    target_link_libraries(mini_kvm PRIVATE ${QT_WMFENGINE_LIB})
endif()

find_library(QT_DSENGINE_LIB dsengine
    PATHS "C:/src/vcpkg/installed/x64-windows-static/plugins/mediaservice"
    NO_DEFAULT_PATH
)
if(QT_DSENGINE_LIB)
    target_link_libraries(mini_kvm PRIVATE ${QT_DSENGINE_LIB})
endif()

# ============================================================
# 第三方头文件路径
# ============================================================
target_include_directories(mini_kvm PRIVATE
    ${CMAKE_SOURCE_DIR}/third_party/cpp_httplib
)

# ============================================================
# Windows 系统库（静态链接 Qt 时需要显式引入）
# ============================================================
target_link_libraries(mini_kvm PRIVATE
    user32 gdi32 shell32 ole32 oleaut32 uuid comdlg32
    advapi32 winspool imm32 ws2_32 opengl32 iphlpapi
)

# Windows Media Foundation（Qt5Multimedia 静态链接依赖）
target_link_libraries(mini_kvm PRIVATE
    mf mfplat mfreadwrite evr strmiids
)

# ============================================================
# 链接器选项：Release 隐藏控制台，Debug 保留控制台
# ============================================================
if(WIN32)
    target_link_options(mini_kvm PRIVATE
        $<$<CONFIG:Release>:/SUBSYSTEM:WINDOWS>
        $<$<CONFIG:Release>:/ENTRY:mainCRTStartup>
        $<$<CONFIG:Debug>:/SUBSYSTEM:CONSOLE>
    )
endif()

# ============================================================
# 编译器选项
# ============================================================
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # clang-cl (MSVC-like) 识别 /utf-8；GNU-like Clang 识别 -finput-charset=UTF-8
    if(CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
        target_compile_options(mini_kvm PRIVATE /W4 /utf-8)
    else()
        target_compile_options(mini_kvm PRIVATE -Wall -Wextra -Wpedantic -finput-charset=UTF-8)
    endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(mini_kvm PRIVATE /W4 /EHsc /utf-8)
endif()
