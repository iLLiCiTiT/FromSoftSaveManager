#include "FlameWidget.h"

#include <QPainter>

// Sprite sheet has a single row of frames
static const int FRAME_WIDTH = 32;
static const int FRAME_HEIGHT = 64;
static const int FRAME_COUNT = 14;
static const int FRAME_INTERVAL_MS = 50;

FlameWidget::FlameWidget(QWidget* parent): QWidget(parent), m_frames(":/ds3_images/autosave_flame") {
    setFixedSize(FRAME_WIDTH, FRAME_HEIGHT);
    m_timer = new QTimer(this);
    m_timer->setInterval(FRAME_INTERVAL_MS);
    connect(m_timer, SIGNAL(timeout()), this, SLOT(onTimeout()));
}

void FlameWidget::onTimeout() {
    m_frame = (m_frame + 1) % FRAME_COUNT;
    update();
}

void FlameWidget::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
    m_timer->start();
}

void FlameWidget::hideEvent(QHideEvent* event) {
    QWidget::hideEvent(event);
    m_timer->stop();
}

void FlameWidget::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    painter.drawPixmap(
        rect(),
        m_frames,
        QRect(m_frame * FRAME_WIDTH, 0, FRAME_WIDTH, FRAME_HEIGHT)
    );
}
