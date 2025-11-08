#include "SekiroWidget.h"

#include <QVBoxLayout>

#include "../BaseGameWidget.h"
#include "../ManageBackupsWidget.h"

SekiroWidget::SekiroWidget(Controller* controller, const QString& saveId, QWidget* parent)
    : BaseGameWidget(controller, saveId, parent)
{
    ManageBackupsButtonsWidget* manageBackupsBtnsWidget = new ManageBackupsButtonsWidget(controller, this);

    QLabel* infoLabel = new QLabel("Sekiro viewer is not implemented yet", this);
    infoLabel->setAlignment(Qt::AlignCenter);

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
