// main.cpp
// Mini-KVM: 视频采集 + CH9329 KVM 控制

#include "controller/DeviceCoordinator.h"
#include "model/VideoCaptureModel.h"
#include "model/SerialPortManager.h"
#include "view/MainWindow.h"

#include <QApplication>
#include <QtPlugin>

// 静态链接 Qt5 时必须导入平台插件
Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin)

int main(int argc, char* argv[])
{
    // 让 Qt 优先尝试 DirectShow 后端，对 UVC 采集卡兼容性更好
    qputenv("QT_MULTIMEDIA_PREFERRED_PLUGINS", "directshow,windowsmediafoundation");

    QApplication app(argc, argv);

    VideoCaptureModel videoModel;
    SerialPortManager serialManager;
    MainWindow window;
    DeviceCoordinator coordinator(&videoModel, &serialManager, &window);

    coordinator.setup();
    window.show();

    return app.exec();
}
