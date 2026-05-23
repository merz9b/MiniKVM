#pragma once

#include <QCamera>
#include <QCameraInfo>
#include <QCameraViewfinderSettings>
#include <QImage>
#include <QObject>
#include <QScopedPointer>
#include <QAbstractVideoSurface>
#include <QList>

class FrameGrabber : public QAbstractVideoSurface {
    Q_OBJECT
public:
    explicit FrameGrabber(QObject* parent = nullptr);

    QList<QVideoFrame::PixelFormat> supportedPixelFormats(
        QAbstractVideoBuffer::HandleType type = QAbstractVideoBuffer::NoHandle) const override;

    bool present(const QVideoFrame& frame) override;

signals:
    void frameReady(QImage image);
};

class VideoCaptureModel : public QObject {
    Q_OBJECT
public:
    explicit VideoCaptureModel(QObject* parent = nullptr);
    ~VideoCaptureModel();

    QList<QCameraInfo> enumerateCameras() const;
    QList<QCameraViewfinderSettings> querySettings(const QCameraInfo& info) const;

    void startCamera(const QCameraInfo& info, const QCameraViewfinderSettings& settings);
    void stopCamera();
    bool isActive() const;

signals:
    void frameReady(QImage image);
    void cameraStarted();
    void cameraStopped();
    void errorOccurred(QString message);

private:
    QScopedPointer<QCamera> m_camera;
    QScopedPointer<FrameGrabber> m_grabber;
};
