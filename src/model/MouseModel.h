#pragma once

#include <QObject>
#include <QPoint>

class SerialPortManager;

class MouseModel : public QObject {
    Q_OBJECT
public:
    explicit MouseModel(SerialPortManager* serial, QObject* parent = nullptr);

    void sendAbsolutePosition(int x, int y, int widgetWidth, int widgetHeight);
    void setButtons(quint8 buttons);
    void sendWheel(int delta);

    // 设置鼠标模式：true=绝对, false=相对
    void setAbsoluteMode(bool absolute);
    bool isAbsoluteMode() const;

private:
    SerialPortManager* m_serial = nullptr;
    bool m_absoluteMode = true;
    quint8 m_currentButtons = 0;

    int m_lastX = 0;
    int m_lastY = 0;
    int m_lastW = 1;
    int m_lastH = 1;
};
