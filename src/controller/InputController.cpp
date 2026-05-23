#include "InputController.h"
#include "../model/MouseModel.h"
#include "../model/KeyboardModel.h"

#include <QMouseEvent>
#include <QWheelEvent>
#include <QWidget>

InputController::InputController(MouseModel* mouse, KeyboardModel* kb, QObject* parent)
    : QObject(parent)
    , m_mouse(mouse)
    , m_kb(kb)
{
}

void InputController::installOn(QWidget* widget)
{
    widget->setMouseTracking(true);
    widget->setFocusPolicy(Qt::StrongFocus);
    widget->installEventFilter(this);
}

void InputController::setThrottleMode(int mode)
{
    switch (mode) {
    case 0:
        m_baseThrottleMs = 10;
        m_smartThrottle = false;
        break;
    case 1:
        m_baseThrottleMs = 5;
        m_smartThrottle = false;
        break;
    case 2:
        m_baseThrottleMs = 10;
        m_smartThrottle = true;
        break;
    default:
        m_baseThrottleMs = 10;
        m_smartThrottle = false;
        break;
    }
}

bool InputController::eventFilter(QObject* watched, QEvent* event)
{
    switch (event->type()) {
    case QEvent::MouseMove: {
        auto* me = static_cast<QMouseEvent*>(event);
        // 按钮释放后 100ms 内，强制使用最后点击坐标，防止微动摇摆破坏双击
        if (m_buttonMask == 0 && m_lastClickTimer.isValid()
            && m_lastClickTimer.elapsed() < 100) {
            m_pendingMousePos = m_lastClickPos;
        } else {
            m_pendingMousePos = me->pos();
        }
        m_hasPendingMousePos = true;
        sendMousePosition(static_cast<QWidget*>(watched));
        break;
    }
    case QEvent::MouseButtonPress: {
        auto* me = static_cast<QMouseEvent*>(event);

        // 双击检测：同一按钮、短时间内、距离近
        bool isDoubleClick = false;
        int elapsed = -1;
        if (m_lastClickButton == me->button() && m_lastClickTimer.isValid()) {
            elapsed = m_lastClickTimer.elapsed();
            if (elapsed < DOUBLE_CLICK_TIME_MS) {
                int dx = qAbs(me->pos().x() - m_lastClickPos.x());
                int dy = qAbs(me->pos().y() - m_lastClickPos.y());
                if (dx <= DOUBLE_CLICK_DIST_PX && dy <= DOUBLE_CLICK_DIST_PX) {
                    isDoubleClick = true;
                }
            }
        }

        if (isDoubleClick) {
            // 双击：锁定为第一次点击的坐标，消除手抖
            m_pendingMousePos = m_lastClickPos;
            m_lastClickTimer.restart(); // 重启 timer，支持连续多击序列
        } else {
            m_lastClickPos = me->pos();
            m_lastClickTimer.restart();
            m_lastClickButton = me->button();
            m_pendingMousePos = me->pos();
        }
        m_hasPendingMousePos = true;

        updateButtonMask(me->button(), true);
        sendMousePosition(static_cast<QWidget*>(watched), true);
        break;
    }
    case QEvent::MouseButtonDblClick: {
        auto* me = static_cast<QMouseEvent*>(event);

        // Qt 双击事件：等价于第二次 MouseButtonPress
        // 如果按钮已经被按下（先收到了 Press），则忽略重复的 DblClick
        quint8 mask = 0;
        switch (me->button()) {
        case Qt::LeftButton:  mask = 0x01; break;
        case Qt::RightButton: mask = 0x02; break;
        case Qt::MiddleButton: mask = 0x04; break;
        default: break;
        }
        if ((m_buttonMask & mask) != 0) {
            break;
        }

        m_pendingMousePos = me->pos();
        m_hasPendingMousePos = true;
        updateButtonMask(me->button(), true);
        sendMousePosition(static_cast<QWidget*>(watched), true);
        break;
    }
    case QEvent::MouseButtonRelease: {
        auto* me = static_cast<QMouseEvent*>(event);

        // 过滤重复释放：该按钮已经释放则忽略
        quint8 mask = 0;
        switch (me->button()) {
        case Qt::LeftButton:  mask = 0x01; break;
        case Qt::RightButton: mask = 0x02; break;
        case Qt::MiddleButton: mask = 0x04; break;
        default: break;
        }
        if ((m_buttonMask & mask) == 0) {
            return true; // 拦截重复释放，防止干扰 Qt 双击检测
        }

        m_lastClickPos = me->pos(); // 记录释放坐标，用于后续 Move 锁定
        m_lastClickTimer.restart();
        updateButtonMask(me->button(), false);
        m_pendingMousePos = me->pos();
        m_hasPendingMousePos = true;
        sendMousePosition(static_cast<QWidget*>(watched), true);
        break;
    }
    case QEvent::Wheel: {
        auto* we = static_cast<QWheelEvent*>(event);
        if (m_mouse) {
            m_mouse->sendWheel(we->angleDelta().y());
        }
        break;
    }
    case QEvent::KeyPress: {
        if (m_kb) {
            m_kb->handleKeyPress(static_cast<QKeyEvent*>(event));
        }
        break;
    }
    case QEvent::KeyRelease: {
        if (m_kb) {
            m_kb->handleKeyRelease(static_cast<QKeyEvent*>(event));
        }
        break;
    }
    default:
        break;
    }

    return false; // 不拦截事件，让控件继续处理
}

void InputController::sendMousePosition(QWidget* widget, bool force)
{
    if (!m_mouse || !m_hasPendingMousePos) {
        return;
    }

    int intervalMs = m_baseThrottleMs;
    if (m_smartThrottle) {
        int dist = qAbs(m_pendingMousePos.x() - m_lastSentPos.x())
                   + qAbs(m_pendingMousePos.y() - m_lastSentPos.y());
        intervalMs = qBound(2, 10 - dist / 3, 10);
    }

    if (force || !m_mouseThrottle.isValid() || m_mouseThrottle.elapsed() > intervalMs) {
        m_mouseThrottle.restart();
        m_lastSentPos = m_pendingMousePos;
        m_mouse->sendAbsolutePosition(m_pendingMousePos.x(), m_pendingMousePos.y(),
                                       widget->width(), widget->height());
        m_hasPendingMousePos = false;
    }
}

void InputController::updateButtonMask(Qt::MouseButton button, bool pressed)
{
    quint8 mask = 0;
    switch (button) {
    case Qt::LeftButton:  mask = 0x01; break;
    case Qt::RightButton: mask = 0x02; break;
    case Qt::MiddleButton: mask = 0x04; break;
    default: return;
    }

    if (pressed) {
        m_buttonMask |= mask;
    } else {
        m_buttonMask &= ~mask;
    }

    if (m_mouse) {
        m_mouse->setButtons(m_buttonMask);
    }
}
