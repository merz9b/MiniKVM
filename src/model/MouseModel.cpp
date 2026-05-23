#include "MouseModel.h"
#include "CH9329Protocol.h"
#include "SerialPortManager.h"

#include <QDebug>

MouseModel::MouseModel(SerialPortManager* serial, QObject* parent)
    : QObject(parent)
    , m_serial(serial)
{
}

void MouseModel::sendAbsolutePosition(int x, int y, int widgetWidth, int widgetHeight)
{
    if (!m_serial || !m_serial->isOpen() || !m_absoluteMode) {
        return;
    }

    m_lastX = x;
    m_lastY = y;
    m_lastW = widgetWidth;
    m_lastH = widgetHeight;

    // 将窗口坐标映射到 0~4095
    int mappedX = (widgetWidth > 0)  ? (x * 4095 / widgetWidth)  : 0;
    int mappedY = (widgetHeight > 0) ? (y * 4095 / widgetHeight) : 0;

    mappedX = qBound(0, mappedX, 4095);
    mappedY = qBound(0, mappedY, 4095);

    QByteArray cmd = CH9329::buildMouseAbs(static_cast<quint16>(mappedX),
                                             static_cast<quint16>(mappedY),
                                             m_currentButtons);
    m_serial->sendAsync(cmd);
}

void MouseModel::setButtons(quint8 buttons)
{
    m_currentButtons = buttons;
    // 绝对模式下不独立发包，由 sendAbsolutePosition / sendWheel 统一携带当前坐标发送
    // 相对模式下同样由 sendAbsolutePosition 统一发送
}

void MouseModel::sendWheel(int delta)
{
    if (!m_serial || !m_serial->isOpen()) {
        return;
    }

    // delta 通常为 ±120，映射到 -1~1
    qint8 wheel = (delta > 0) ? 1 : ((delta < 0) ? -1 : 0);
    if (wheel == 0) {
        return;
    }

    if (m_absoluteMode) {
        int mappedX = 0, mappedY = 0;
        if (m_lastW > 0) mappedX = qBound(0, m_lastX * 4095 / m_lastW, 4095);
        if (m_lastH > 0) mappedY = qBound(0, m_lastY * 4095 / m_lastH, 4095);
        QByteArray cmd = CH9329::buildMouseAbs(static_cast<quint16>(mappedX),
                                                 static_cast<quint16>(mappedY),
                                                 m_currentButtons,
                                                 static_cast<quint8>(wheel));
        m_serial->sendAsync(cmd);
    }
}

void MouseModel::setAbsoluteMode(bool absolute)
{
    m_absoluteMode = absolute;
}

bool MouseModel::isAbsoluteMode() const
{
    return m_absoluteMode;
}
