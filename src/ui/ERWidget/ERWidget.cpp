#include "ERWidget.h"

#include <QPainter>
#include <qevent.h>

#include "../BaseGameWidget.h"

ERWidget::ERWidget(Controller* controller, const QString& saveId, QWidget* parent)
    : BaseGameWidget(controller, saveId, parent)
{};

void ERWidget::refresh() {
    // TODO implement
};


void ERWidget::paintEvent(QPaintEvent* event) {
    QPainter painter = QPainter(this);
    painter.setClipRect(event->rect());
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0, 0, 0));
    painter.drawRect(rect());
    QPixmap pix = QPixmap(":/er_images/bg");
    QSize currentSize = size() * 0.7;
    QPixmap scaled = pix.scaled(currentSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    const int x = (width() - scaled.width()) / 2;
    const int y = (height() - scaled.height()) / 2;
    painter.drawPixmap(x, y, scaled);

    QPixmap pixOverlay = QPixmap(":/er_images/bg_overlay");
    painter.drawPixmap(0, 0, pixOverlay.scaled(size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
}