#include "SerialPortManager.h"

#include <QDebug>

SerialPortManager::SerialPortManager(QObject* parent)
    : QObject(parent)
{
}

SerialPortManager::~SerialPortManager() = default;

QList<QSerialPortInfo> SerialPortManager::enumeratePorts() const
{
    return QSerialPortInfo::availablePorts();
}

bool SerialPortManager::openPort(const QString& portName, int baudRate)
{
    closePort();

    m_serial.reset(new QSerialPort(this));
    m_serial->setPortName(portName);
    m_serial->setBaudRate(baudRate);
    m_serial->setDataBits(QSerialPort::Data8);
    m_serial->setParity(QSerialPort::NoParity);
    m_serial->setStopBits(QSerialPort::OneStop);
    m_serial->setFlowControl(QSerialPort::NoFlowControl);

    connect(m_serial.data(), &QSerialPort::readyRead, this, &SerialPortManager::onReadyRead);
    connect(m_serial.data(), QOverload<QSerialPort::SerialPortError>::of(&QSerialPort::error),
        this, [this](QSerialPort::SerialPortError error) {
            if (error != QSerialPort::NoError && m_serial) {
                emit errorOccurred(m_serial->errorString());
            }
        });

    if (!m_serial->open(QIODevice::ReadWrite)) {
        emit errorOccurred(QString::fromUtf8(u8"无法打开串口: ") + m_serial->errorString());
        m_serial.reset();
        return false;
    }

    emit connectionChanged(true);
    return true;
}

void SerialPortManager::closePort()
{
    if (m_serial) {
        m_serial->close();
        m_serial.reset();
        emit connectionChanged(false);
    }
}

bool SerialPortManager::isOpen() const
{
    return m_serial && m_serial->isOpen();
}

bool SerialPortManager::send(const QByteArray& data)
{
    if (!m_serial || !m_serial->isOpen()) {
        emit errorOccurred(QString::fromUtf8(u8"串口未打开"));
        return false;
    }

    qint64 written = m_serial->write(data);
    m_serial->flush();
    return written == data.size();
}

bool SerialPortManager::sendAsync(const QByteArray& data)
{
    // 当前实现同 send，后续可加入队列
    return send(data);
}

QString SerialPortManager::portName() const
{
    return m_serial ? m_serial->portName() : QString();
}

int SerialPortManager::baudRate() const
{
    return m_serial ? m_serial->baudRate() : 0;
}

void SerialPortManager::onReadyRead()
{
    if (!m_serial) {
        return;
    }
    QByteArray data = m_serial->readAll();
    if (!data.isEmpty()) {
        emit dataReceived(data);
    }
}
