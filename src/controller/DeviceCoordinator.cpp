#include "DeviceCoordinator.h"
#include "InputController.h"
#include "HttpServerController.h"
#include "../model/VideoCaptureModel.h"
#include "../model/SerialPortManager.h"
#include "../model/MouseModel.h"
#include "../model/KeyboardModel.h"
#include "../model/CH9329Protocol.h"
#include "../view/MainWindow.h"
#include "../view/VideoWidget.h"
#include "../view/HttpConfigDialog.h"

#include <QApplication>
#include <QBuffer>
#include <QDateTime>
#include <QFileDialog>
#include <QScreen>
#include <QSerialPortInfo>
#include <QVideoFrame>
#include <QWindow>

DeviceCoordinator::DeviceCoordinator(VideoCaptureModel* videoModel,
                                     SerialPortManager* serial,
                                     MainWindow* view,
                                     QObject* parent)
    : QObject(parent)
    , m_videoModel(videoModel)
    , m_serial(serial)
    , m_view(view)
{
    m_mouse = new MouseModel(serial, this);
    m_kb = new KeyboardModel(serial, this);
    m_input = new InputController(m_mouse, m_kb, this);
    m_httpServer = new HttpServerController(this);
}

void DeviceCoordinator::setup()
{
    // 视频信号
    connect(m_videoModel, &VideoCaptureModel::frameReady, this, &DeviceCoordinator::onFrameReady);
    connect(m_videoModel, &VideoCaptureModel::errorOccurred, this, [this](const QString& msg) {
        m_view->setStatusText(QString::fromUtf8(u8"视频错误: ") + msg);
        m_view->setConnectEnabled(true);
        m_view->setDisconnectEnabled(false);
    });
    connect(m_view, &MainWindow::videoSourceChanged, this, &DeviceCoordinator::onVideoSourceChanged);
    connect(m_view, &MainWindow::connectRequested, this, &DeviceCoordinator::onConnectRequested);
    connect(m_view, &MainWindow::disconnectRequested, this, &DeviceCoordinator::onDisconnectRequested);
    connect(m_view, &MainWindow::captureRequested, this, &DeviceCoordinator::onCaptureRequested);
    connect(m_view, &MainWindow::toggleFullscreenRequested, this, &DeviceCoordinator::onToggleFullscreen);

    // 串口信号
    connect(m_serial, &SerialPortManager::dataReceived, this, &DeviceCoordinator::onSerialDataReceived);
    connect(m_serial, &SerialPortManager::errorOccurred, this, &DeviceCoordinator::onSerialError);
    connect(m_serial, &SerialPortManager::connectionChanged, this, &DeviceCoordinator::onSerialConnectionChanged);
    connect(m_view, &MainWindow::serialPortChanged, this, &DeviceCoordinator::onSerialPortChanged);
    connect(m_view, &MainWindow::serialConnectRequested, this, &DeviceCoordinator::onSerialConnectRequested);
    connect(m_view, &MainWindow::serialDisconnectRequested, this, &DeviceCoordinator::onSerialDisconnectRequested);
    connect(m_view, &MainWindow::baudRateChanged, this, &DeviceCoordinator::onBaudRateChanged);
    connect(m_view, &MainWindow::httpConfigRequested, this, &DeviceCoordinator::onHttpConfigRequested);

    // HTTP server 状态
    connect(m_httpServer, &HttpServerController::started, this, [this](quint16 port) {
        m_view->setHttpStatusText(QString::fromUtf8(u8"HTTP: 运行中(%1)").arg(port));
    });
    connect(m_httpServer, &HttpServerController::stopped, this, [this]() {
        m_view->setHttpStatusText(QString::fromUtf8(u8"HTTP: 停止"));
    });
    connect(m_httpServer, &HttpServerController::errorOccurred, this, [this](const QString& msg) {
        m_view->setHttpStatusText(QString::fromUtf8(u8"HTTP: 错误"));
        m_view->setStatusText(QString::fromUtf8(u8"HTTP 错误: ") + msg);
    });

    // 初始化
    refreshVideoSources();
    refreshSerialPorts();
    m_view->setConnectEnabled(!m_cameraList.isEmpty());
    m_view->setSerialConnectEnabled(!m_serialList.isEmpty());

    // 在 VideoWidget 上安装输入拦截器
    m_input->installOn(m_view->videoWidget());

    // 设置 HTTP 截图回调（在 server 线程编码，避免阻塞主线程或死锁）
    m_httpServer->setCaptureCallback([this]() -> QByteArray {
        QImage frame;
        {
            QMutexLocker lock(&m_frameMutex);
            frame = m_lastFrame.copy();
        }
        if (frame.isNull()) {
            return QByteArray();
        }
        QByteArray pngData;
        QBuffer buffer(&pngData);
        buffer.open(QIODevice::WriteOnly);
        frame.save(&buffer, "PNG");
        return pngData;
    });

    // 窗口关闭时执行清理
    connect(m_view, &MainWindow::aboutToClose, this, &DeviceCoordinator::shutdown);
}

// ------------------------------------------------------------------
// 视频
// ------------------------------------------------------------------

void DeviceCoordinator::refreshVideoSources()
{
    m_cameraList = m_videoModel->enumerateCameras();
    QStringList names;
    for (const auto& info : m_cameraList) {
        names.append(info.description());
    }
    m_view->setVideoSources(names);
}

void DeviceCoordinator::onVideoSourceChanged(int index)
{
    if (index < 0 || index >= m_cameraList.size()) {
        m_view->clearSettings();
        m_view->setConnectEnabled(false);
        return;
    }

    m_view->setStatusText(QString::fromUtf8(u8"正在查询 ") + m_cameraList[index].description() + QString::fromUtf8(u8" 的支持格式..."));
    updateAvailableSettings();
}

void DeviceCoordinator::updateAvailableSettings()
{
    int camIndex = m_view->currentVideoSourceIndex();
    if (camIndex < 0 || camIndex >= m_cameraList.size()) {
        return;
    }

    m_settingsList = m_videoModel->querySettings(m_cameraList[camIndex]);

    QStringList items;
    for (const auto& s : m_settingsList) {
        QString fmtName;
        switch (s.pixelFormat()) {
        case QVideoFrame::Format_YUYV: fmtName = "YUYV"; break;
        case QVideoFrame::Format_UYVY: fmtName = "UYVY"; break;
        case QVideoFrame::Format_RGB32: fmtName = "RGB32"; break;
        case QVideoFrame::Format_ARGB32: fmtName = "ARGB32"; break;
        case QVideoFrame::Format_YUV420P: fmtName = "YUV420P"; break;
        case QVideoFrame::Format_Jpeg: fmtName = "MJPEG"; break;
        default: fmtName = QString("Fmt_%1").arg(s.pixelFormat()); break;
        }
        QString text = QString::fromUtf8(u8"%1x%2 | %3 | %4fps")
                           .arg(s.resolution().width())
                           .arg(s.resolution().height())
                           .arg(fmtName)
                           .arg(s.maximumFrameRate(), 0, 'f', 1);
        items.append(text);
    }

    m_view->setSettings(items);
    m_view->setConnectEnabled(!m_settingsList.isEmpty());
    m_view->setStatusText(QString::fromUtf8(u8"已加载 %1 种设置").arg(m_settingsList.size()));
}

void DeviceCoordinator::onConnectRequested()
{
    int camIndex = m_view->currentVideoSourceIndex();
    int settingIndex = m_view->currentSettingIndex();

    if (camIndex < 0 || camIndex >= m_cameraList.size()) {
        return;
    }
    if (settingIndex < 0 || settingIndex >= m_settingsList.size()) {
        return;
    }

    m_view->setStatusText(QString::fromUtf8(u8"正在连接视频: ") + m_cameraList[camIndex].description());
    m_videoModel->startCamera(m_cameraList[camIndex], m_settingsList[settingIndex]);
    m_view->setConnectEnabled(false);
    m_view->setDisconnectEnabled(true);
}

void DeviceCoordinator::onDisconnectRequested()
{
    m_videoModel->stopCamera();
    m_view->videoWidget()->setImage(QImage());
    m_view->setConnectEnabled(true);
    m_view->setDisconnectEnabled(false);
    m_view->setStatusText(QString::fromUtf8(u8"视频已断开"));
}

void DeviceCoordinator::onFrameReady(QImage image)
{
    m_view->videoWidget()->setImage(image);
    {
        QMutexLocker lock(&m_frameMutex);
        m_lastFrame = image.copy();  // 深拷贝，供 HTTP server 线程安全读取
    }

    QString status = QString::fromUtf8(u8"%1x%2 | 视频:%3 | 串口:%4")
                         .arg(image.width())
                         .arg(image.height())
                         .arg(m_cameraList.value(m_view->currentVideoSourceIndex()).description())
                         .arg(m_serial->isOpen() ? m_serial->portName() : QString::fromUtf8(u8"关闭"));
    m_view->setStatusText(status);
}

// ------------------------------------------------------------------
// 串口 / CH9329
// ------------------------------------------------------------------

void DeviceCoordinator::refreshSerialPorts()
{
    m_serialList = m_serial->enumeratePorts();
    QStringList names;
    for (const auto& info : m_serialList) {
        QString text = info.portName();
        if (!info.description().isEmpty()) {
            text += " - " + info.description();
        }
        if (!info.serialNumber().isEmpty()) {
            text += " (" + info.serialNumber() + ")";
        }
        names.append(text);
    }
    m_view->setSerialPorts(names);
    m_view->setSerialConnectEnabled(!m_serialList.isEmpty());
}

void DeviceCoordinator::onBaudRateChanged(int mode)
{
    if (m_input) {
        m_input->setThrottleMode(mode);
    }
}

void DeviceCoordinator::onHttpConfigRequested()
{
    if (!m_httpDialog) {
        m_httpDialog = new HttpConfigDialog(m_view);
        connect(m_httpDialog, &HttpConfigDialog::applyRequested,
                this, &DeviceCoordinator::onHttpApplyRequested);
    }
    m_httpDialog->show();
    m_httpDialog->raise();
    m_httpDialog->activateWindow();
}

void DeviceCoordinator::onHttpApplyRequested(int port, bool enable)
{
    if (enable) {
        m_httpServer->start(static_cast<quint16>(port));
    } else {
        m_httpServer->stop();
    }
}

void DeviceCoordinator::onSerialPortChanged(int index)
{
    Q_UNUSED(index)
    // 串口选择变化时无需特殊处理
}

void DeviceCoordinator::onSerialConnectRequested()
{
    int idx = m_view->currentSerialPortIndex();
    if (idx < 0 || idx >= m_serialList.size()) {
        return;
    }

    QString portName = m_serialList[idx].portName();
    int baud = m_view->baudRate();
    m_view->setStatusText(QString::fromUtf8(u8"正在打开串口: ") + portName + QString(" @ %1").arg(baud));

    if (m_serial->openPort(portName, baud)) {
        m_view->setSerialConnectEnabled(false);
        m_view->setSerialDisconnectEnabled(true);
        m_view->setStatusText(QString::fromUtf8(u8"串口已打开: ") + portName);
        m_input->setThrottleMode(m_view->throttleMode());

        // 发送 GET_INFO 探测 CH9329
        queryCh9329Info();
    } else {
        // 配置的波特率失败则尝试 115200
        m_view->setStatusText(QString::fromUtf8(u8"打开失败，尝试 115200..."));
        if (m_serial->openPort(portName, 115200)) {
            m_view->setSerialConnectEnabled(false);
            m_view->setSerialDisconnectEnabled(true);
            m_view->setStatusText(QString::fromUtf8(u8"串口已打开: ") + portName + " @ 115200");
            m_input->setThrottleMode(m_view->throttleMode());
            queryCh9329Info();
        } else {
            m_view->setStatusText(QString::fromUtf8(u8"串口打开失败"));
        }
    }
}

void DeviceCoordinator::onSerialDisconnectRequested()
{
    m_kb->releaseAllKeys();
    m_serial->closePort();
    m_view->setSerialConnectEnabled(true);
    m_view->setSerialDisconnectEnabled(false);
    m_view->setStatusText(QString::fromUtf8(u8"串口已关闭"));
}

void DeviceCoordinator::onSerialDataReceived(QByteArray data)
{
    // 简单打印接收到的原始数据，后续可扩展为 CH9329 响应解析
    qDebug() << "Serial RX:" << data.toHex(' ').toUpper();
}

void DeviceCoordinator::onSerialError(QString message)
{
    m_view->setStatusText(QString::fromUtf8(u8"串口错误: ") + message);
}

void DeviceCoordinator::onSerialConnectionChanged(bool connected)
{
    if (!connected) {
        m_view->setSerialConnectEnabled(true);
        m_view->setSerialDisconnectEnabled(false);
    }
}

void DeviceCoordinator::queryCh9329Info()
{
    if (!m_serial->isOpen()) {
        return;
    }
    QByteArray cmd = CH9329::buildGetInfo();
    m_serial->send(cmd);
}

// ------------------------------------------------------------------
// 通用
// ------------------------------------------------------------------

void DeviceCoordinator::onCaptureRequested()
{
    auto image = m_view->videoWidget()->grab().toImage();
    if (image.isNull()) {
        return;
    }

    QString defaultName = QString::fromUtf8(u8"capture_%1.png")
                              .arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));
    QString fileName = QFileDialog::getSaveFileName(m_view,
        QString::fromUtf8(u8"保存截图"),
        defaultName,
        QString::fromUtf8(u8"PNG 图片 (*.png);;JPEG 图片 (*.jpg)"));

    if (!fileName.isEmpty()) {
        image.save(fileName);
    }
}

void DeviceCoordinator::onToggleFullscreen()
{
    m_isFullscreen = !m_isFullscreen;
    m_view->setFullscreenMode(m_isFullscreen);
}

void DeviceCoordinator::shutdown()
{
    // 先停 HTTP server，再释放键盘（防止目标机卡键），再关串口，最后停视频
    if (m_httpServer) {
        m_httpServer->stop();
    }
    if (m_kb) {
        m_kb->releaseAllKeys();
    }
    if (m_serial && m_serial->isOpen()) {
        m_serial->closePort();
    }
    if (m_videoModel) {
        m_videoModel->stopCamera();
    }
}
