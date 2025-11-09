#include "SekiroWidget.h"

#include <QPainter>
#include <QVBoxLayout>

#include "../BaseGameWidget.h"
#include "../ManageBackupsWidget.h"

SekiroWidget::SekiroWidget(Controller* controller, const QString& saveId, QWidget* parent)
    : BaseGameWidget(controller, saveId, parent)
{
    m_bgPix = QPixmap(":/sekiro_images/bg");
    ManageBackupsButtonsWidget* manageBackupsBtnsWidget = new ManageBackupsButtonsWidget(controller, this);

    QLabel* infoLabel = new QLabel("Sekiro viewer is not implemented yet", this);
    infoLabel->setAlignment(Qt::AlignCenter);
    infoLabel->setAttribute(Qt::WA_TranslucentBackground, true);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 24, 24, 24);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(infoLabel, 3);
    mainLayout->addWidget(manageBackupsBtnsWidget, 0);
    mainLayout->addStretch(1);

    connect(manageBackupsBtnsWidget, SIGNAL(showBackupsRequested()), this, SIGNAL(showBackupsRequested()));
}

void SekiroWidget::refresh() {
    // TODO implement
}

void SekiroWidget::paintEvent(QPaintEvent* event) {
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