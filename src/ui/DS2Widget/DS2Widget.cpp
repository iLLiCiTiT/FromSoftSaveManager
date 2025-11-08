#include "DS2Widget.h"

#include <QVBoxLayout>

#include "../BaseGameWidget.h"
#include "../ManageBackupsWidget.h"

DS2Widget::DS2Widget(Controller* controller, const QString& saveId, QWidget* parent)
    : BaseGameWidget(controller, saveId, parent)
{
    ManageBackupsButtonsWidget* manageBackupsBtnsWidget = new ManageBackupsButtonsWidget(controller, this);

    QLabel* infoLabel = new QLabel("Dark Souls 2 - SOTFS viewer is not implemented yet", this);
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
