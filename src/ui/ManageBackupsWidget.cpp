#include "ManageBackupsWidget.h"

#include <QHBoxLayout>
#include <QPushButton>

CreateBackupDialog::CreateBackupDialog(QWidget* parent): QDialog(parent) {
    setWindowTitle("Create save backup");
    setModal(true);

    m_backupNameinput = new QLineEdit(this);
    m_backupNameinput->setPlaceholderText("Backup name..");
    m_backupNameinput->setToolTip(
        "Label for the backup, leave empty to use default"
    );

    QWidget* btnsWidget = new QWidget(this);

    QPushButton* createBtn = new QPushButton("Create", btnsWidget);
    createBtn->setDefault(true);
    createBtn->setToolTip("Create backup");

    QPushButton* cancelBtn = new QPushButton("Cancel", btnsWidget);
    cancelBtn->setToolTip("Cancel");

    QHBoxLayout* btnsLayout = new QHBoxLayout(btnsWidget);
    btnsLayout->setContentsMargins(0, 0, 0, 0);
    btnsLayout->addStretch(1);
    btnsLayout->addWidget(createBtn, 0);
    btnsLayout->addWidget(cancelBtn, 0);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->addWidget(m_backupNameinput, 0);
    layout->addStretch(1);
    layout->addWidget(btnsWidget, 0);

    connect(createBtn, SIGNAL(clicked()), this, SLOT(accept()));
    connect(cancelBtn, SIGNAL(clicked()), this, SLOT(reject()));
}

QString CreateBackupDialog::getBackupName() {
    return m_backupNameinput->text();
}

// Common UI widget for all games
ManageBackupsButtonsWidget::ManageBackupsButtonsWidget(Controller* controller, QWidget* parent): QFrame(parent), m_controller(controller)
{
    m_hotkeysLabel = new QLabel(this);
    m_hotkeysLabel->setAlignment(Qt::AlignCenter);

    QWidget* btnsWidget = new QWidget(this);
    QPushButton* createBackupBtn = new QPushButton("Create backup", btnsWidget);
    createBackupBtn->setToolTip("Create a backup of the current save");

    QPushButton* showBackupsBtn = new QPushButton("Show backups", btnsWidget);
    showBackupsBtn->setToolTip("Show available backups");

    QPushButton* openBackupDirBtn = new QPushButton("Open Backup dir", btnsWidget);
    openBackupDirBtn->setToolTip("Open Backup directory");

    QHBoxLayout* btnsLayout = new QHBoxLayout(btnsWidget);
    btnsLayout->setContentsMargins(0, 0, 0, 0);
    btnsLayout->addStretch(1);
    btnsLayout->addWidget(createBackupBtn, 0);
    btnsLayout->addWidget(showBackupsBtn, 0);
    btnsLayout->addWidget(openBackupDirBtn, 0);
    btnsLayout->addStretch(1);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_hotkeysLabel, 0);
    layout->addWidget(btnsWidget, 0);
    layout->addStretch(1);

    this->setAttribute(Qt::WA_TranslucentBackground, true);
    m_hotkeysLabel->setAttribute(Qt::WA_TranslucentBackground, true);
    btnsWidget->setAttribute(Qt::WA_TranslucentBackground, true);

    connect(createBackupBtn, SIGNAL(clicked()), this, SLOT(onCreateBackup()));
    connect(showBackupsBtn, SIGNAL(clicked()), this, SLOT(onShowBackups()));
    connect(openBackupDirBtn, SIGNAL(clicked()), this, SLOT(onOpenBackupDir()));
    connect(m_controller, SIGNAL(hotkeysConfigChanged()), this, SLOT(onHotkeysChange()));

    onHotkeysChange();
};

void ManageBackupsButtonsWidget::onCreateBackup() {
    CreateBackupDialog dialog = CreateBackupDialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        QString label = dialog.getBackupName();
        m_controller->createManualBackup(label);
    }
}

void ManageBackupsButtonsWidget::onShowBackups() {
    // TODO implement
}

void ManageBackupsButtonsWidget::onOpenBackupDir() {
    m_controller->openBackupDir();
}

void ManageBackupsButtonsWidget::onHotkeysChange() {
    auto hotkeysConfig = m_controller->getHotkeysConfig();
    auto& quicksave = hotkeysConfig.quickSaveHotkey;
    auto& quickload = hotkeysConfig.quickLoadHotkey;
    if (quicksave.key() == Qt::Key_unknown && quickload.key() == Qt::Key_unknown) {
        m_hotkeysLabel->setVisible(false);
        return;
    }
    QString label;
    if (quicksave.key() != Qt::Key_unknown) {
        label.push_back(QKeySequence(quicksave).toString());
        label.push_back(" to quicksave");
    }
    if (quickload.key() != Qt::Key_unknown) {
        if (!label.isEmpty()) label.push_back(" and ");
        label.push_back(QKeySequence(quickload).toString());
        label.push_back(" to quickload");
    }
    label.push_front("<i>");
    label.push_back("</i>");
    m_hotkeysLabel->setVisible(true);
    m_hotkeysLabel->setText(label);
}
