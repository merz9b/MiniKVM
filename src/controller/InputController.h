#pragma once

#include <QElapsedTimer>
#include <QObject>
#include <QPoint>

class MouseModel;
class KeyboardModel;

class InputController : public QObject {
    Q_OBJECT
public:
    explicit InputController(MouseModel* mouse, KeyboardModel* kb, QObject* parent = nullptr);

    void installOn(QWidget* widget);
    void setThrottleMode(int mode); // 0=标准(10ms), 1=高速(5ms), 2=智能

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void sendMousePosition(QWidget* widget, bool force = false);
    void updateButtonMask(Qt::MouseButton button, bool pressed);

    MouseModel* m_mouse = nullptr;
    KeyboardModel* m_kb = nullptr;
    QElapsedTimer m_mouseThrottle;
    QPoint m_pendingMousePos;
    bool m_hasPendingMousePos = false;
    quint8 m_buttonMask = 0;

    // 节流控制
    int m_baseThrottleMs = 10;   // 基础节流间隔 ms
    bool m_smartThrottle = false;// 是否启用智能调节
    QPoint m_lastSentPos;        // 上一次实际发送的坐标

    // 双击坐标锁定：消除快速点击时手抖导致的坐标偏移
    QPoint m_lastClickPos;
    QElapsedTimer m_lastClickTimer;
    Qt::MouseButton m_lastClickButton = Qt::NoButton;
    static constexpr int DOUBLE_CLICK_DIST_PX = 30;  // 约 3% 屏幕宽度，覆盖手抖
    static constexpr int DOUBLE_CLICK_TIME_MS = 500; // 与 Windows 默认双击时间一致
};
