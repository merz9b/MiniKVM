#pragma once

#include <QDialog>

class QSpinBox;
class QPushButton;
class QLabel;

class HttpConfigDialog : public QDialog {
    Q_OBJECT
public:
    explicit HttpConfigDialog(QWidget* parent = nullptr);

signals:
    void applyRequested(int port, bool enable);

private slots:
    void onToggleClicked();
    void updateUi(bool running);

private:
    void loadSettings();
    void saveSettings();

    QSpinBox* m_portSpin = nullptr;
    QPushButton* m_toggleBtn = nullptr;
    QLabel* m_statusLabel = nullptr;

    bool m_isRunning = false;
};
