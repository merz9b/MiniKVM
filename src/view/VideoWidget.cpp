#include "VideoWidget.h"

#include <QPainter>

VideoWidget::VideoWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumSize(320, 240);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setAttribute(Qt::WA_OpaquePaintEvent);
}

void VideoWidget::setImage(const QImage& image)
{
    m_currentImage = image;

    // 空图像（Disconnect）时强制刷新，避免残留画面
    if (image.isNull()) {
        m_fpsTimer.invalidate();
        update();
        return;
    }

    // 限制重绘到约 30fps，避免 60fps 输入压垮 UI 线程
    if (!m_fpsTimer.isValid() || m_fpsTimer.elapsed() > 33) {
        m_fpsTimer.restart();
        update();
    }
}

void VideoWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    painter.fillRect(rect(), Qt::black);

    if (m_currentImage.isNull()) {
        painter.setPen(Qt::white);
        painter.drawText(rect(), Qt::AlignCenter,
            QString::fromUtf8(u8"无视频信号\n请选择视频源后点击 Connect"));
        return;
    }

    // 用 QPainter::drawImage 做硬件加速缩放，避免 CPU 创建中间 QImage
    QRect target = m_currentImage.rect();
    target.setSize(m_currentImage.size().scaled(rect().size(), Qt::KeepAspectRatio));
    target.moveCenter(rect().center());
    painter.drawImage(target, m_currentImage);
}
