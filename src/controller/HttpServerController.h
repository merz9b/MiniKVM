#pragma once

#include <QByteArray>
#include <QObject>
#include <atomic>
#include <functional>
#include <thread>

namespace httplib {
class Server;
}

class HttpServerController : public QObject {
    Q_OBJECT
public:
    explicit HttpServerController(QObject* parent = nullptr);
    ~HttpServerController() override;

    void start(quint16 port);
    void stop();
    bool isRunning() const;
    void setCaptureCallback(std::function<QByteArray()> callback);

signals:
    void started(quint16 port);
    void stopped();
    void errorOccurred(QString message);

private:
    void runServer(quint16 port);

    std::atomic<bool> m_running{false};
    std::thread m_serverThread;
    httplib::Server* m_server = nullptr;

    std::function<QByteArray()> m_captureCallback;
};
