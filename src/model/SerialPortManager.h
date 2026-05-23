#pragma once

#include <QByteArray>
#include <QObject>
#include <QScopedPointer>
#include <QSerialPort>
#include <QSerialPortInfo>

class SerialPortManager : public QObject {
    Q_OBJECT
public:
    explicit SerialPortManager(QObject* parent = nullptr);
    ~SerialPortManager();

    QList<QSerialPortInfo> enumeratePorts() const;
    bool openPort(const QString& portName, int baudRate = 9600);
    void closePort();
    bool isOpen() const;

    bool send(const QByteArray& data);
    bool sendAsync(const QByteArray& data);

    QString portName() const;
    int baudRate() const;

signals:
    void dataReceived(QByteArray data);
    void connectionChanged(bool connected);
    void errorOccurred(QString message);

private slots:
    void onReadyRead();

private:
    QScopedPointer<QSerialPort> m_serial;
};
