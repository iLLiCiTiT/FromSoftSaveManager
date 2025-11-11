#include "DS2Widget.h"

#include <QPainter>
#include <QVBoxLayout>

#include "../BaseGameWidget.h"
#include "../ManageBackupsWidget.h"

DS2Widget::DS2Widget(Controller* controller, const QString& saveId, QWidget* parent)
    : BaseGameWidget(controller, saveId, parent)
{
    setAttribute(Qt::WA_TranslucentBackground, true);

    m_bgPix = QPixmap(":/ds2_images/bg");

    ManageBackupsButtonsWidget* manageBackupsBtnsWidget = new ManageBackupsButtonsWidget(controller, this);

    QLabel* infoLabel = new QLabel("Dark Souls 2 - SOTFS viewer is not implemented yet", this);
    infoLabel->setAttribute(Qt::WA_TranslucentBackground, true);
    infoLabel->setAlignment(Qt::AlignCenter);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 24, 24, 24);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(infoLabel, 3);
    mainLayout->addWidget(manageBackupsBtnsWidget, 0);
    mainLayout->addStretch(1);

    connect(manageBackupsBtnsWidget, SIGNAL(showBackupsRequested()), this, SIGNAL(showBackupsRequested()));
}

void DS2Widget::refresh() {
    // TODO implement
}

void DS2Widget::paintEvent(QPaintEvent* event) {
    QPainter painter = QPainter(this);
    painter.setClipRect(event->rect());
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(6, 5, 7));
    const QRect targetRect = rect();
    painter.drawRect(targetRect);
    QPixmap scaled = m_bgPix.scaled(size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    // center vertically
    const int x = (width() - scaled.width()) / 2;
    const int y = (height() - scaled.height()) / 2;
    painter.drawPixmap(x, y, scaled);
}