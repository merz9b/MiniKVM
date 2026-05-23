#pragma once

#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QMainWindow>
#include <QPushButton>
#include <QToolBar>

class VideoWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

    void setVideoSources(const QStringList& sources);
    int currentVideoSourceIndex() const;

    void clearSettings();
    void setSettings(const QStringList& items);
    int currentSettingIndex() const;

    void setConnectEnabled(bool enabled);
    void setDisconnectEnabled(bool enabled);
    void setStatusText(const QString& text);
    void setHttpStatusText(const QString& text);
    void setFullscreenMode(bool fullscreen);

    void setSerialPorts(const QStringList& ports);
    int currentSerialPortIndex() const;
    void setSerialConnectEnabled(bool enabled);
    void setSerialDisconnectEnabled(bool enabled);
    int baudRate() const;
    int throttleMode() const;

    VideoWidget* videoWidget() const;

protected:
    void closeEvent(QCloseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

signals:
    void aboutToClose();
    void videoSourceChanged(int index);
    void settingChanged(int index);
    void connectRequested();
    void disconnectRequested();
    void captureRequested();
    void toggleFullscreenRequested();
    void serialPortChanged(int index);
    void serialConnectRequested();
    void serialDisconnectRequested();
    void hideLocalMouseChanged(bool hidden);
    void baudRateChanged(int mode);
    void httpConfigRequested();

private:
    void setupUi();

    VideoWidget* m_videoWidget = nullptr;
    QComboBox* m_videoCombo = nullptr;
    QComboBox* m_settingCombo = nullptr;
    QPushButton* m_connectBtn = nullptr;
    QPushButton* m_disconnectBtn = nullptr;
    QPushButton* m_fullscreenBtn = nullptr;
    QLabel* m_statusLabel = nullptr;
    QLabel* m_httpStatusLabel = nullptr;
    QToolBar* m_toolBar = nullptr;

    QComboBox* m_serialCombo = nullptr;
    QPushButton* m_serialConnectBtn = nullptr;
    QPushButton* m_serialDisconnectBtn = nullptr;
    QComboBox* m_baudCombo = nullptr;
    QCheckBox* m_hideLocalMouseCheck = nullptr;
};
