#pragma once

#include <QCameraInfo>
#include <QCameraViewfinderSettings>
#include <QImage>
#include <QMutex>
#include <QObject>
#include <QSerialPortInfo>
#include <QTimer>

class VideoCaptureModel;
class SerialPortManager;
class MouseModel;
class KeyboardModel;
class InputController;
class MainWindow;
class HttpServerController;
class HttpConfigDialog;

class DeviceCoordinator : public QObject {
    Q_OBJECT
public:
    DeviceCoordinator(VideoCaptureModel* videoModel,
                      SerialPortManager* serial,
                      MainWindow* view,
                      QObject* parent = nullptr);

    void setup();

private slots:
    // 视频
    void onVideoSourceChanged(int index);
    void onConnectRequested();
    void onDisconnectRequested();
    void onFrameReady(QImage image);

    // 串口 / CH9329
    void onSerialPortChanged(int index);
    void onSerialConnectRequested();
    void onSerialDisconnectRequested();
    void onSerialDataReceived(QByteArray data);
    void onSerialError(QString message);
    void onSerialConnectionChanged(bool connected);
    void refreshSerialPorts();
    void onBaudRateChanged(int mode);

    // HTTP
    void onHttpConfigRequested();
    void onHttpApplyRequested(int port, bool enable);

    // 通用
    void onCaptureRequested();
    void onToggleFullscreen();
    void shutdown();

private:
    void updateAvailableSettings();
    void refreshVideoSources();
    void queryCh9329Info();

    VideoCaptureModel* m_videoModel = nullptr;
    SerialPortManager* m_serial = nullptr;
    MouseModel* m_mouse = nullptr;
    KeyboardModel* m_kb = nullptr;
    InputController* m_input = nullptr;
    MainWindow* m_view = nullptr;
    HttpServerController* m_httpServer = nullptr;
    HttpConfigDialog* m_httpDialog = nullptr;

    QList<QCameraInfo> m_cameraList;
    QList<QCameraViewfinderSettings> m_settingsList;
    QList<QSerialPortInfo> m_serialList;
    QImage m_lastFrame;
    QMutex m_frameMutex;

    bool m_isFullscreen = false;
    QTimer m_ch9329QueryTimer;
};
