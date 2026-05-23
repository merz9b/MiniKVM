#pragma once

#include <QByteArray>
#include <QObject>

namespace CH9329 {

// 命令码
constexpr quint8 CMD_GET_INFO         = 0x01;
constexpr quint8 CMD_SEND_KB          = 0x02;
constexpr quint8 CMD_SEND_KB_REL      = 0x03; // 释放按键（非标准，部分固件支持）
constexpr quint8 CMD_MOUSE_ABS        = 0x04;
constexpr quint8 CMD_MOUSE_REL        = 0x05;
constexpr quint8 CMD_MOUSE_WHEEL      = 0x06;
constexpr quint8 CMD_GET_CFG          = 0x08;
constexpr quint8 CMD_SET_CFG          = 0x09;
constexpr quint8 CMD_SET_USB_STRING   = 0x0B;
constexpr quint8 CMD_SET_DEFAULT_CFG  = 0x0C;
constexpr quint8 CMD_RESET            = 0x0F;

// 帧头
constexpr quint8 HEAD_H = 0x57;
constexpr quint8 HEAD_L = 0xAB;
constexpr quint8 ADDR   = 0x00;

// 校验和计算：Sum = 0x57 + 0xAB + 0x00 + Cmd + Len + sum(Data)
quint8 checksum(const QByteArray& data);

// 构造完整帧：<Head 0x57AB> <Addr 0x00> <Cmd> <Len> [Data] <Sum>
QByteArray buildFrame(quint8 cmd, const QByteArray& data);

// 鼠标按钮掩码
constexpr quint8 MOUSE_LEFT   = 0x01;
constexpr quint8 MOUSE_RIGHT  = 0x02;
constexpr quint8 MOUSE_MIDDLE = 0x04;

// 构造鼠标绝对模式命令 (0x04)
// x, y: 0~4095
QByteArray buildMouseAbs(quint16 x, quint16 y, quint8 buttons = 0, quint8 wheel = 0);

// 构造鼠标相对模式命令 (0x05)
QByteArray buildMouseRel(qint8 dx, qint8 dy, quint8 buttons = 0);

// 构造键盘命令 (0x02)
// modifier: bit0=L_Ctrl, bit1=L_Shift, bit2=L_Alt, bit3=L_GUI
// keys: 最多 6 个 HID 码
QByteArray buildKeyboard(quint8 modifier, const QByteArray& keys);

// 构造 GET_INFO (0x01)
QByteArray buildGetInfo();

// 构造 GET_CFG (0x08)
QByteArray buildGetCfg();

// 构造 RESET (0x0F)
QByteArray buildReset();

// 构造 SET_DEFAULT_CFG (0x0C)
QByteArray buildSetDefaultCfg();

} // namespace CH9329
