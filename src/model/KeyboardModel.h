#pragma once

#include <QKeyEvent>
#include <QObject>

class SerialPortManager;

class KeyboardModel : public QObject {
    Q_OBJECT
public:
    explicit KeyboardModel(SerialPortManager* serial, QObject* parent = nullptr);

    void handleKeyPress(QKeyEvent* event);
    void handleKeyRelease(QKeyEvent* event);
    void releaseAllKeys();

private:
    void sendKeyboardReport();
    quint8 qtKeyToHid(int qtKey) const;
    void updateModifiers(int qtKey, bool pressed);

    SerialPortManager* m_serial = nullptr;
    quint8 m_modifiers = 0;      // 修饰键状态
    QList<int> m_pressedKeys;     // 当前按下的 HID 码（最多 6 个）
};
