#include "KeyboardModel.h"
#include "CH9329Protocol.h"
#include "SerialPortManager.h"

#include <QDebug>

KeyboardModel::KeyboardModel(SerialPortManager* serial, QObject* parent)
    : QObject(parent)
    , m_serial(serial)
{
}

void KeyboardModel::handleKeyPress(QKeyEvent* event)
{
    if (!m_serial || !m_serial->isOpen()) {
        return;
    }

    int qtKey = event->key();
    updateModifiers(qtKey, true);

    quint8 hid = qtKeyToHid(qtKey);
    if (hid == 0) {
        // 修饰键（Ctrl/Shift/Alt/Meta）没有 HID scan code，通过 modifier 字节发送
        if (qtKey == Qt::Key_Control || qtKey == Qt::Key_Shift ||
            qtKey == Qt::Key_Alt || qtKey == Qt::Key_Meta) {
            sendKeyboardReport();
        }
        return;
    }

    // 已在按下列表中则忽略（自动重复）
    if (m_pressedKeys.contains(hid)) {
        return;
    }

    if (m_pressedKeys.size() >= 6) {
        // HID 报告最多 6 个键，满了则忽略新键
        return;
    }

    m_pressedKeys.append(hid);
    sendKeyboardReport();
}

void KeyboardModel::handleKeyRelease(QKeyEvent* event)
{
    if (!m_serial || !m_serial->isOpen()) {
        return;
    }

    int qtKey = event->key();
    updateModifiers(qtKey, false);

    quint8 hid = qtKeyToHid(qtKey);
    if (hid == 0) {
        if (qtKey == Qt::Key_Control || qtKey == Qt::Key_Shift ||
            qtKey == Qt::Key_Alt || qtKey == Qt::Key_Meta) {
            sendKeyboardReport();
        }
        return;
    }

    m_pressedKeys.removeAll(hid);
    sendKeyboardReport();
}

void KeyboardModel::releaseAllKeys()
{
    m_pressedKeys.clear();
    m_modifiers = 0;
    if (m_serial && m_serial->isOpen()) {
        sendKeyboardReport();
    }
}

void KeyboardModel::sendKeyboardReport()
{
    QByteArray keys;
    for (int k : m_pressedKeys) {
        keys.append(static_cast<char>(k));
    }
    QByteArray cmd = CH9329::buildKeyboard(m_modifiers, keys);
    m_serial->sendAsync(cmd);
}

void KeyboardModel::updateModifiers(int qtKey, bool pressed)
{
    quint8 mask = 0;
    switch (qtKey) {
    case Qt::Key_Control: mask = 0x01; break; // Left Ctrl
    case Qt::Key_Shift:   mask = 0x02; break; // Left Shift
    case Qt::Key_Alt:     mask = 0x04; break; // Left Alt
    case Qt::Key_Meta:    mask = 0x08; break; // Left GUI
    default: return;
    }

    if (pressed) {
        m_modifiers |= mask;
    } else {
        m_modifiers &= ~mask;
    }
}

quint8 KeyboardModel::qtKeyToHid(int qtKey) const
{
    // 字母 A-Z
    if (qtKey >= Qt::Key_A && qtKey <= Qt::Key_Z) {
        return static_cast<quint8>(0x04 + (qtKey - Qt::Key_A));
    }

    // 数字 1-9, 0
    if (qtKey >= Qt::Key_1 && qtKey <= Qt::Key_9) {
        return static_cast<quint8>(0x1E + (qtKey - Qt::Key_1));
    }
    if (qtKey == Qt::Key_0) {
        return 0x27;
    }

    // 功能键 F1-F12
    if (qtKey >= Qt::Key_F1 && qtKey <= Qt::Key_F12) {
        return static_cast<quint8>(0x3A + (qtKey - Qt::Key_F1));
    }

    switch (qtKey) {
    case Qt::Key_Return:
    case Qt::Key_Enter:     return 0x28;
    case Qt::Key_Escape:    return 0x29;
    case Qt::Key_Backspace: return 0x2A;
    case Qt::Key_Tab:       return 0x2B;
    case Qt::Key_Space:     return 0x2C;
    case Qt::Key_Minus:     return 0x2D;
    case Qt::Key_Equal:     return 0x2E;
    case Qt::Key_BracketLeft:  return 0x2F;
    case Qt::Key_BracketRight: return 0x30;
    case Qt::Key_Backslash: return 0x31;
    case Qt::Key_Semicolon: return 0x33;
    case Qt::Key_Apostrophe: return 0x34;
    case Qt::Key_QuoteLeft: return 0x35; // Grave
    case Qt::Key_Comma:     return 0x36;
    case Qt::Key_Period:    return 0x37;
    case Qt::Key_Slash:     return 0x38;
    case Qt::Key_CapsLock:  return 0x39;

    // 导航键
    case Qt::Key_Print:     return 0x46;
    case Qt::Key_ScrollLock: return 0x47;
    case Qt::Key_Pause:     return 0x48;
    case Qt::Key_Insert:    return 0x49;
    case Qt::Key_Home:      return 0x4A;
    case Qt::Key_PageUp:    return 0x4B;
    case Qt::Key_Delete:    return 0x4C;
    case Qt::Key_End:       return 0x4D;
    case Qt::Key_PageDown:  return 0x4E;
    case Qt::Key_Right:     return 0x4F;
    case Qt::Key_Left:      return 0x50;
    case Qt::Key_Down:      return 0x51;
    case Qt::Key_Up:        return 0x52;

    // 小键盘 (Qt 中小键盘 / * - + 与主键盘共用枚举值)
    case Qt::Key_NumLock:   return 0x53;
    default: break;
    }

    return 0;
}
