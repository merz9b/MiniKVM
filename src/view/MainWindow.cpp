#include "MainWindow.h"
#include "VideoWidget.h"

#include <QAction>
#include <QCloseEvent>
#include <QKeyEvent>
#include <QMenu>
#include <QMenuBar>
#include <QStatusBar>
#include <QToolBar>
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setupUi();
    setWindowTitle(QString::fromUtf8(u8"Mini-KVM"));
    // 根据工具栏控件总宽度估算初始宽度，避免按钮被折叠
    // 视频源(180) + 设置(200) + Connect(70) + Disconnect(80) + 全屏(50) + 隐藏鼠标(100)
    // + 串口(120) + 波特率(120) + Open(55) + Close(55) + 分隔符/间距(~200) ≈ 1230
    resize(1320, 768);
}

void MainWindow::setupUi()
{
    auto* central = new QWidget(this);
    auto* layout = new QVBoxLayout(central);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_videoWidget = new VideoWidget(central);
    layout->addWidget(m_videoWidget, 1);

    setCentralWidget(central);

    auto* toolbar = addToolBar(QString::fromUtf8(u8"主工具栏"));
    toolbar->setMovable(false);
    m_toolBar = toolbar;

    // 视频源
    m_videoCombo = new QComboBox(toolbar);
    m_videoCombo->setMinimumWidth(180);
    connect(m_videoCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
        this, &MainWindow::videoSourceChanged);
    toolbar->addWidget(m_videoCombo);

    toolbar->addSeparator();

    // 设置 (分辨率/格式/帧率)
    m_settingCombo = new QComboBox(toolbar);
    m_settingCombo->setMinimumWidth(200);
    connect(m_settingCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
        this, &MainWindow::settingChanged);
    toolbar->addWidget(m_settingCombo);

    toolbar->addSeparator();

    // Connect / Disconnect (Video)
    m_connectBtn = new QPushButton(QString::fromUtf8(u8"Connect"), toolbar);
    m_connectBtn->setFixedWidth(70);
    connect(m_connectBtn, &QPushButton::clicked, this, &MainWindow::connectRequested);
    toolbar->addWidget(m_connectBtn);

    m_disconnectBtn = new QPushButton(QString::fromUtf8(u8"Disconnect"), toolbar);
    m_disconnectBtn->setFixedWidth(80);
    m_disconnectBtn->setEnabled(false);
    connect(m_disconnectBtn, &QPushButton::clicked, this, &MainWindow::disconnectRequested);
    toolbar->addWidget(m_disconnectBtn);

    toolbar->addSeparator();

    // 全屏
    m_fullscreenBtn = new QPushButton(QString::fromUtf8(u8"全屏"), toolbar);
    m_fullscreenBtn->setFixedWidth(50);
    m_fullscreenBtn->setToolTip(QString::fromUtf8(u8"全屏 / 退出全屏 (Ctrl+M)"));
    connect(m_fullscreenBtn, &QPushButton::clicked, this, &MainWindow::toggleFullscreenRequested);
    toolbar->addWidget(m_fullscreenBtn);

    toolbar->addSeparator();

    // 隐藏本机鼠标
    m_hideLocalMouseCheck = new QCheckBox(QString::fromUtf8(u8"隐藏本机鼠标"), toolbar);
    connect(m_hideLocalMouseCheck, &QCheckBox::stateChanged, this, [this](int state) {
        bool hidden = (state == Qt::Checked);
        if (hidden) {
            m_videoWidget->setCursor(Qt::BlankCursor);
        } else {
            m_videoWidget->unsetCursor();
        }
        emit hideLocalMouseChanged(hidden);
    });
    toolbar->addWidget(m_hideLocalMouseCheck);

    toolbar->addSeparator();

    // 串口选择
    m_serialCombo = new QComboBox(toolbar);
    m_serialCombo->setMinimumWidth(120);
    connect(m_serialCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
        this, &MainWindow::serialPortChanged);
    toolbar->addWidget(m_serialCombo);

    // 波特率 / 节流模式
    m_baudCombo = new QComboBox(toolbar);
    m_baudCombo->setMinimumWidth(120);
    m_baudCombo->addItem(QString::fromUtf8(u8"9600 (标准)"), 9600);
    m_baudCombo->addItem(QString::fromUtf8(u8"57600 (高速)"), 57600);
    m_baudCombo->addItem(QString::fromUtf8(u8"115200 (智能)"), 115200);
    connect(m_baudCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
        this, [this](int index) { emit baudRateChanged(index); });
    toolbar->addWidget(m_baudCombo);

    // 串口 Open / Close
    m_serialConnectBtn = new QPushButton(QString::fromUtf8(u8"Open"), toolbar);
    m_serialConnectBtn->setFixedWidth(55);
    connect(m_serialConnectBtn, &QPushButton::clicked, this, &MainWindow::serialConnectRequested);
    toolbar->addWidget(m_serialConnectBtn);

    m_serialDisconnectBtn = new QPushButton(QString::fromUtf8(u8"Close"), toolbar);
    m_serialDisconnectBtn->setFixedWidth(55);
    m_serialDisconnectBtn->setEnabled(false);
    connect(m_serialDisconnectBtn, &QPushButton::clicked, this, &MainWindow::serialDisconnectRequested);
    toolbar->addWidget(m_serialDisconnectBtn);

    // 菜单栏：配置 -> HTTP配置
    auto* menuConfig = menuBar()->addMenu(QString::fromUtf8(u8"配置(&C)"));
    auto* actHttpConfig = menuConfig->addAction(QString::fromUtf8(u8"HTTP配置"));
    connect(actHttpConfig, &QAction::triggered, this, &MainWindow::httpConfigRequested);

    // 菜单栏：工具 -> 截图
    auto* menuTools = menuBar()->addMenu(QString::fromUtf8(u8"工具(&T)"));
    auto* actCapture = menuTools->addAction(QString::fromUtf8(u8"截图"));
    connect(actCapture, &QAction::triggered, this, &MainWindow::captureRequested);

    m_statusLabel = new QLabel(QString::fromUtf8(u8"就绪 — 请选择视频源和串口"), this);
    statusBar()->addWidget(m_statusLabel);

    m_httpStatusLabel = new QLabel(QString::fromUtf8(u8"HTTP: 停止"), this);
    statusBar()->addPermanentWidget(m_httpStatusLabel);
}

void MainWindow::setVideoSources(const QStringList& sources)
{
    m_videoCombo->clear();
    m_videoCombo->addItems(sources);
}

int MainWindow::currentVideoSourceIndex() const
{
    return m_videoCombo->currentIndex();
}

void MainWindow::clearSettings()
{
    m_settingCombo->clear();
}

void MainWindow::setSettings(const QStringList& items)
{
    m_settingCombo->clear();
    m_settingCombo->addItems(items);
}

int MainWindow::currentSettingIndex() const
{
    return m_settingCombo->currentIndex();
}

void MainWindow::setConnectEnabled(bool enabled)
{
    m_connectBtn->setEnabled(enabled);
}

void MainWindow::setDisconnectEnabled(bool enabled)
{
    m_disconnectBtn->setEnabled(enabled);
}

void MainWindow::setStatusText(const QString& text)
{
    m_statusLabel->setText(text);
}

void MainWindow::setHttpStatusText(const QString& text)
{
    m_httpStatusLabel->setText(text);
}

VideoWidget* MainWindow::videoWidget() const
{
    return m_videoWidget;
}

void MainWindow::setFullscreenMode(bool fullscreen)
{
    if (fullscreen) {
        menuBar()->hide();
        m_toolBar->hide();
        statusBar()->hide();
        showFullScreen();
    } else {
        menuBar()->show();
        m_toolBar->show();
        statusBar()->show();
        showNormal();
    }
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    emit aboutToClose();
    event->accept();
}

void MainWindow::keyPressEvent(QKeyEvent* event)
{
    if (event->modifiers() == Qt::ControlModifier && event->key() == Qt::Key_M) {
        emit toggleFullscreenRequested();
    } else {
        QMainWindow::keyPressEvent(event);
    }
}

void MainWindow::setSerialPorts(const QStringList& ports)
{
    m_serialCombo->clear();
    m_serialCombo->addItems(ports);
}

int MainWindow::currentSerialPortIndex() const
{
    return m_serialCombo->currentIndex();
}

void MainWindow::setSerialConnectEnabled(bool enabled)
{
    m_serialConnectBtn->setEnabled(enabled);
}

void MainWindow::setSerialDisconnectEnabled(bool enabled)
{
    m_serialDisconnectBtn->setEnabled(enabled);
}

int MainWindow::baudRate() const
{
    return m_baudCombo->currentData().toInt();
}

int MainWindow::throttleMode() const
{
    return m_baudCombo->currentIndex();
}
