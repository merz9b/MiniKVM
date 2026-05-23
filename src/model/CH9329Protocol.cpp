#include "CH9329Protocol.h"

namespace CH9329 {

quint8 checksum(const QByteArray& data)
{
    quint8 sum = 0;
    for (char c : data) {
        sum += static_cast<quint8>(c);
    }
    return sum;
}

QByteArray buildFrame(quint8 cmd, const QByteArray& data)
{
    QByteArray frame;
    frame.reserve(5 + data.size() + 1);
    frame.append(static_cast<char>(HEAD_H));
    frame.append(static_cast<char>(HEAD_L));
    frame.append(static_cast<char>(ADDR));
    frame.append(static_cast<char>(cmd));
    frame.append(static_cast<char>(static_cast<quint8>(data.size())));
    frame.append(data);
    frame.append(static_cast<char>(checksum(frame)));
    return frame;
}

QByteArray buildMouseAbs(quint16 x, quint16 y, quint8 buttons, quint8 wheel)
{
    QByteArray data;
    data.reserve(7);
    data.append(static_cast<char>(0x02)); // 绝对模式标识
    data.append(static_cast<char>(buttons));
    data.append(static_cast<char>(x & 0xFF));
    data.append(static_cast<char>((x >> 8) & 0xFF));
    data.append(static_cast<char>(y & 0xFF));
    data.append(static_cast<char>((y >> 8) & 0xFF));
    data.append(static_cast<char>(wheel));
    return buildFrame(CMD_MOUSE_ABS, data);
}

QByteArray buildMouseRel(qint8 dx, qint8 dy, quint8 buttons)
{
    QByteArray data;
    data.reserve(5);
    data.append(static_cast<char>(0x01)); // 相对模式子命令
    data.append(static_cast<char>(buttons));
    data.append(static_cast<char>(static_cast<quint8>(dx)));
    data.append(static_cast<char>(static_cast<quint8>(dy)));
    data.append(static_cast<char>(0x00)); // wheel
    return buildFrame(CMD_MOUSE_REL, data);
}

QByteArray buildKeyboard(quint8 modifier, const QByteArray& keys)
{
    QByteArray data;
    data.reserve(8);
    data.append(static_cast<char>(modifier));
    data.append(static_cast<char>(0x00)); // reserved
    for (int i = 0; i < 6; ++i) {
        if (i < keys.size()) {
            data.append(static_cast<char>(static_cast<quint8>(keys.at(i))));
        } else {
            data.append(static_cast<char>(0x00));
        }
    }
    return buildFrame(CMD_SEND_KB, data);
}

QByteArray buildGetInfo()
{
    return buildFrame(CMD_GET_INFO, QByteArray());
}

QByteArray buildGetCfg()
{
    return buildFrame(CMD_GET_CFG, QByteArray());
}

QByteArray buildReset()
{
    return buildFrame(CMD_RESET, QByteArray());
}

QByteArray buildSetDefaultCfg()
{
    return buildFrame(CMD_SET_DEFAULT_CFG, QByteArray());
}

} // namespace CH9329
