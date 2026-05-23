#include "VideoCaptureModel.h"

#include <QCameraViewfinderSettings>
#include <QPainter>
#include <QVideoFrame>

FrameGrabber::FrameGrabber(QObject* parent)
    : QAbstractVideoSurface(parent)
{
}

QList<QVideoFrame::PixelFormat> FrameGrabber::supportedPixelFormats(
    QAbstractVideoBuffer::HandleType type) const
{
    Q_UNUSED(type)
    return {
        QVideoFrame::Format_ARGB32,
        QVideoFrame::Format_ARGB32_Premultiplied,
        QVideoFrame::Format_RGB32,
        QVideoFrame::Format_RGB24,
        QVideoFrame::Format_RGB565,
        QVideoFrame::Format_RGB555,
        QVideoFrame::Format_ARGB8565_Premultiplied,
        QVideoFrame::Format_BGRA32,
        QVideoFrame::Format_BGRA32_Premultiplied,
        QVideoFrame::Format_BGR32,
        QVideoFrame::Format_BGR24,
        QVideoFrame::Format_BGR565,
        QVideoFrame::Format_BGR555,
        QVideoFrame::Format_BGRA5658_Premultiplied,
        QVideoFrame::Format_AYUV444,
        QVideoFrame::Format_AYUV444_Premultiplied,
        QVideoFrame::Format_YUV444,
        QVideoFrame::Format_YUV420P,
        QVideoFrame::Format_YV12,
        QVideoFrame::Format_UYVY,
        QVideoFrame::Format_YUYV,
        QVideoFrame::Format_NV12,
        QVideoFrame::Format_NV21,
        QVideoFrame::Format_IMC1,
        QVideoFrame::Format_IMC2,
        QVideoFrame::Format_IMC3,
        QVideoFrame::Format_IMC4,
        QVideoFrame::Format_Y8,
        QVideoFrame::Format_Y16,
        QVideoFrame::Format_Jpeg,
        QVideoFrame::Format_CameraRaw,
        QVideoFrame::Format_AdobeDng,
    };
}

bool FrameGrabber::present(const QVideoFrame& frame)
{
    if (!frame.isValid()) {
        return false;
    }

    QVideoFrame cloneFrame(frame);
    if (!cloneFrame.map(QAbstractVideoBuffer::ReadOnly)) {
        return false;
    }

    QImage image;
    switch (cloneFrame.pixelFormat()) {
    case QVideoFrame::Format_ARGB32:
        image = QImage(cloneFrame.bits(), cloneFrame.width(), cloneFrame.height(), cloneFrame.bytesPerLine(), QImage::Format_ARGB32);
        break;
    case QVideoFrame::Format_ARGB32_Premultiplied:
        image = QImage(cloneFrame.bits(), cloneFrame.width(), cloneFrame.height(), cloneFrame.bytesPerLine(), QImage::Format_ARGB32_Premultiplied);
        break;
    case QVideoFrame::Format_RGB32:
        image = QImage(cloneFrame.bits(), cloneFrame.width(), cloneFrame.height(), cloneFrame.bytesPerLine(), QImage::Format_RGB32);
        break;
    case QVideoFrame::Format_RGB24:
        image = QImage(cloneFrame.bits(), cloneFrame.width(), cloneFrame.height(), cloneFrame.bytesPerLine(), QImage::Format_RGB888);
        break;
    case QVideoFrame::Format_ARGB8565_Premultiplied:
        image = QImage(cloneFrame.bits(), cloneFrame.width(), cloneFrame.height(), cloneFrame.bytesPerLine(), QImage::Format_ARGB8565_Premultiplied);
        break;
    case QVideoFrame::Format_BGRA32:
        image = QImage(cloneFrame.bits(), cloneFrame.width(), cloneFrame.height(), cloneFrame.bytesPerLine(), QImage::Format_ARGB32);
        break;
    case QVideoFrame::Format_BGRA32_Premultiplied:
        image = QImage(cloneFrame.bits(), cloneFrame.width(), cloneFrame.height(), cloneFrame.bytesPerLine(), QImage::Format_ARGB32_Premultiplied);
        break;
    case QVideoFrame::Format_BGR32:
        image = QImage(cloneFrame.bits(), cloneFrame.width(), cloneFrame.height(), cloneFrame.bytesPerLine(), QImage::Format_RGB32);
        break;
    case QVideoFrame::Format_BGR24:
        image = QImage(cloneFrame.bits(), cloneFrame.width(), cloneFrame.height(), cloneFrame.bytesPerLine(), QImage::Format_RGB888);
        break;
    case QVideoFrame::Format_YUV420P:
    case QVideoFrame::Format_YV12:
        image = QImage(cloneFrame.width(), cloneFrame.height(), QImage::Format_RGB32);
        {
            const int w = cloneFrame.width();
            const int h = cloneFrame.height();
            const uchar* yPlane = cloneFrame.bits();
            const uchar* uPlane = yPlane + w * h;
            const uchar* vPlane = uPlane + (w / 2) * (h / 2);
            const int yBpl = cloneFrame.bytesPerLine();
            const int uvBpl = yBpl / 2;
            const int dstBpl = image.bytesPerLine();
            for (int row = 0; row < h; ++row) {
                const uchar* yp = yPlane + row * yBpl;
                const uchar* up = uPlane + (row / 2) * uvBpl;
                const uchar* vp = vPlane + (row / 2) * uvBpl;
                QRgb* dst = reinterpret_cast<QRgb*>(image.bits() + row * dstBpl);
                for (int x = 0; x < w; ++x) {
                    int yv = yp[x] - 16;
                    int uv = up[x / 2] - 128;
                    int vv = vp[x / 2] - 128;
                    int r = (298 * yv + 409 * vv + 128) >> 8;
                    int g = (298 * yv - 100 * uv - 208 * vv + 128) >> 8;
                    int b = (298 * yv + 516 * uv + 128) >> 8;
                    r = r < 0 ? 0 : (r > 255 ? 255 : r);
                    g = g < 0 ? 0 : (g > 255 ? 255 : g);
                    b = b < 0 ? 0 : (b > 255 ? 255 : b);
                    dst[x] = qRgb(r, g, b);
                }
            }
        }
        break;
    case QVideoFrame::Format_YUYV: {
        image = QImage(cloneFrame.width(), cloneFrame.height(), QImage::Format_RGB32);
        const uchar* src = cloneFrame.bits();
        const int srcBpl = cloneFrame.bytesPerLine();
        const int dstBpl = image.bytesPerLine();
        const int w = cloneFrame.width();
        for (int row = 0; row < cloneFrame.height(); ++row) {
            const uchar* s = src + row * srcBpl;
            QRgb* d = reinterpret_cast<QRgb*>(image.bits() + row * dstBpl);
            for (int x = 0; x < w; x += 2) {
                int y0 = s[0];
                int u  = s[1];
                int y1 = s[2];
                int v  = s[3];
                s += 4;

                int c0 = y0 - 16;
                int c1 = y1 - 16;
                int du = u - 128;
                int dv = v - 128;

                int r0 = (298 * c0 + 409 * dv + 128) >> 8;
                int g0 = (298 * c0 - 100 * du - 208 * dv + 128) >> 8;
                int b0 = (298 * c0 + 516 * du + 128) >> 8;
                r0 = r0 < 0 ? 0 : (r0 > 255 ? 255 : r0);
                g0 = g0 < 0 ? 0 : (g0 > 255 ? 255 : g0);
                b0 = b0 < 0 ? 0 : (b0 > 255 ? 255 : b0);
                *d++ = qRgb(r0, g0, b0);

                int r1 = (298 * c1 + 409 * dv + 128) >> 8;
                int g1 = (298 * c1 - 100 * du - 208 * dv + 128) >> 8;
                int b1 = (298 * c1 + 516 * du + 128) >> 8;
                r1 = r1 < 0 ? 0 : (r1 > 255 ? 255 : r1);
                g1 = g1 < 0 ? 0 : (g1 > 255 ? 255 : g1);
                b1 = b1 < 0 ? 0 : (b1 > 255 ? 255 : b1);
                *d++ = qRgb(r1, g1, b1);
            }
        }
        break;
    }
    case QVideoFrame::Format_UYVY: {
        image = QImage(cloneFrame.width(), cloneFrame.height(), QImage::Format_RGB32);
        const uchar* src = cloneFrame.bits();
        const int srcBpl = cloneFrame.bytesPerLine();
        const int dstBpl = image.bytesPerLine();
        const int w = cloneFrame.width();
        for (int row = 0; row < cloneFrame.height(); ++row) {
            const uchar* s = src + row * srcBpl;
            QRgb* d = reinterpret_cast<QRgb*>(image.bits() + row * dstBpl);
            for (int x = 0; x < w; x += 2) {
                int u  = s[0];
                int y0 = s[1];
                int v  = s[2];
                int y1 = s[3];
                s += 4;

                int c0 = y0 - 16;
                int c1 = y1 - 16;
                int du = u - 128;
                int dv = v - 128;

                int r0 = (298 * c0 + 409 * dv + 128) >> 8;
                int g0 = (298 * c0 - 100 * du - 208 * dv + 128) >> 8;
                int b0 = (298 * c0 + 516 * du + 128) >> 8;
                r0 = r0 < 0 ? 0 : (r0 > 255 ? 255 : r0);
                g0 = g0 < 0 ? 0 : (g0 > 255 ? 255 : g0);
                b0 = b0 < 0 ? 0 : (b0 > 255 ? 255 : b0);
                *d++ = qRgb(r0, g0, b0);

                int r1 = (298 * c1 + 409 * dv + 128) >> 8;
                int g1 = (298 * c1 - 100 * du - 208 * dv + 128) >> 8;
                int b1 = (298 * c1 + 516 * du + 128) >> 8;
                r1 = r1 < 0 ? 0 : (r1 > 255 ? 255 : r1);
                g1 = g1 < 0 ? 0 : (g1 > 255 ? 255 : g1);
                b1 = b1 < 0 ? 0 : (b1 > 255 ? 255 : b1);
                *d++ = qRgb(r1, g1, b1);
            }
        }
        break;
    }
    default:
        cloneFrame.unmap();
        return false;
    }

    if (!image.isNull()) {
        // 对于直接映射 QVideoFrame bits() 的格式，必须在 unmap 前深拷贝，
        // 否则 cloneFrame.unmap() 后 QImage 指向的内存可能失效。
        switch (cloneFrame.pixelFormat()) {
        case QVideoFrame::Format_ARGB32:
        case QVideoFrame::Format_ARGB32_Premultiplied:
        case QVideoFrame::Format_RGB32:
        case QVideoFrame::Format_RGB24:
        case QVideoFrame::Format_ARGB8565_Premultiplied:
        case QVideoFrame::Format_BGRA32:
        case QVideoFrame::Format_BGRA32_Premultiplied:
        case QVideoFrame::Format_BGR32:
        case QVideoFrame::Format_BGR24:
            image = image.copy();
            break;
        default:
            break;
        }
        emit frameReady(image);
    }

    cloneFrame.unmap();
    return true;
}

VideoCaptureModel::VideoCaptureModel(QObject* parent)
    : QObject(parent)
    , m_grabber(new FrameGrabber(this))
{
    connect(m_grabber.data(), &FrameGrabber::frameReady, this, &VideoCaptureModel::frameReady);
}

VideoCaptureModel::~VideoCaptureModel()
{
    stopCamera();
}

QList<QCameraInfo> VideoCaptureModel::enumerateCameras() const
{
    return QCameraInfo::availableCameras();
}

QList<QCameraViewfinderSettings> VideoCaptureModel::querySettings(const QCameraInfo& info) const
{
    QCamera tempCamera(info);
    tempCamera.load();
    auto settings = tempCamera.supportedViewfinderSettings();
    tempCamera.unload();
    return settings;
}

void VideoCaptureModel::startCamera(const QCameraInfo& info, const QCameraViewfinderSettings& settings)
{
    stopCamera();

    m_camera.reset(new QCamera(info));
    connect(m_camera.data(), QOverload<QCamera::Error>::of(&QCamera::error),
        this, [this](QCamera::Error error) {
            Q_UNUSED(error)
            emit errorOccurred(m_camera->errorString());
        });

    m_camera->setViewfinderSettings(settings);
    m_camera->setViewfinder(m_grabber.data());
    m_camera->start();

    emit cameraStarted();
}

void VideoCaptureModel::stopCamera()
{
    if (m_camera) {
        m_camera->stop();
        m_camera.reset();
        emit cameraStopped();
    }
}

bool VideoCaptureModel::isActive() const
{
    return m_camera && m_camera->state() == QCamera::ActiveState;
}
