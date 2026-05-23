#pragma once

#include <QElapsedTimer>
#include <QImage>
#include <QWidget>

class VideoWidget : public QWidget {
    Q_OBJECT
public:
    explicit VideoWidget(QWidget* parent = nullptr);

public slots:
    void setImage(const QImage& image);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QImage m_currentImage;
    QElapsedTimer m_fpsTimer;
};
