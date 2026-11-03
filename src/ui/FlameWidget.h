#pragma once

#include <QPixmap>
#include <QTimer>
#include <QWidget>

// Looping flame animation from Dark Souls III autosave icon
class FlameWidget: public QWidget {
    Q_OBJECT
public:
    explicit FlameWidget(QWidget* parent);

protected:
    void paintEvent(QPaintEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

private slots:
    void onTimeout();

private:
    QTimer* m_timer;
    QPixmap m_frames;
    int m_frame = 0;
};
