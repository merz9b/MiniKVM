#include "HttpConfigDialog.h"

#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>

HttpConfigDialog::HttpConfigDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QString::fromUtf8(u8"HTTP 配置"));
    setFixedSize(300, 150);

    auto* layout = new QGridLayout(this);

    layout->addWidget(new QLabel(QString::fromUtf8(u8"监听端口:"), this), 0, 0);

    m_portSpin = new QSpinBox(this);
    m_portSpin->setRange(1024, 65535);
    m_portSpin->setValue(8765);
    layout->addWidget(m_portSpin, 0, 1);

    layout->addWidget(new QLabel(QString::fromUtf8(u8"状态:"), this), 1, 0);

    m_statusLabel = new QLabel(QString::fromUtf8(u8"已停止"), this);
    layout->addWidget(m_statusLabel, 1, 1);

    m_toggleBtn = new QPushButton(QString::fromUtf8(u8"启用"), this);
    connect(m_toggleBtn, &QPushButton::clicked, this, &HttpConfigDialog::onToggleClicked);
    layout->addWidget(m_toggleBtn, 2, 0, 1, 2);

    loadSettings();
}

void HttpConfigDialog::onToggleClicked()
{
    int port = m_portSpin->value();
    bool enable = !m_isRunning;
    emit applyRequested(port, enable);
    updateUi(enable);
    if (enable) {
        saveSettings();
    }
}

void HttpConfigDialog::updateUi(bool running)
{
    m_isRunning = running;
    if (running) {
        m_statusLabel->setText(QString::fromUtf8(u8"运行中"));
        m_toggleBtn->setText(QString::fromUtf8(u8"停止"));
        m_portSpin->setEnabled(false);
    } else {
        m_statusLabel->setText(QString::fromUtf8(u8"已停止"));
        m_toggleBtn->setText(QString::fromUtf8(u8"启用"));
        m_portSpin->setEnabled(true);
    }
}

void HttpConfigDialog::loadSettings()
{
    QSettings settings("MiniKVM", "HttpConfig");
    int port = settings.value("port", 8765).toInt();
    m_portSpin->setValue(qBound(1024, port, 65535));
}

void HttpConfigDialog::saveSettings()
{
    QSettings settings("MiniKVM", "HttpConfig");
    settings.setValue("port", m_portSpin->value());
}
