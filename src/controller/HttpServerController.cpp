#include "HttpServerController.h"
#include "httplib.h"

HttpServerController::HttpServerController(QObject* parent)
    : QObject(parent)
{
}

HttpServerController::~HttpServerController()
{
    stop();
}

void HttpServerController::start(quint16 port)
{
    if (m_running.load()) {
        return;
    }
    m_running.store(true);
    m_serverThread = std::thread(&HttpServerController::runServer, this, port);
}

void HttpServerController::stop()
{
    if (!m_running.load()) {
        return;
    }
    m_running.store(false);
    if (m_server) {
        m_server->stop();
    }
    if (m_serverThread.joinable()) {
        m_serverThread.join();
    }
    emit stopped();
}

bool HttpServerController::isRunning() const
{
    return m_running.load();
}

void HttpServerController::setCaptureCallback(std::function<QByteArray()> callback)
{
    m_captureCallback = std::move(callback);
}

void HttpServerController::runServer(quint16 port)
{
    httplib::Server svr;
    m_server = &svr;

    svr.Get("/capture", [this](const httplib::Request&, httplib::Response& res) {
        QByteArray data = m_captureCallback ? m_captureCallback() : QByteArray();
        if (data.isEmpty()) {
            res.status = 503;
            res.set_content("{\"error\":\"no capture data\"}", "application/json");
            return;
        }
        res.set_content(data.data(), data.size(), "image/png");
    });

    emit started(port);

    if (!svr.listen("127.0.0.1", port)) {
        emit errorOccurred(QString::fromUtf8(u8"HTTP Server 启动失败，端口可能被占用"));
    }

    m_server = nullptr;
}
