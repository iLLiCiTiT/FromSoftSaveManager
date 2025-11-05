#include "ManageBackupsWidget.h"

#include <QSplitter>
#include <QTreeView>
#include <QGraphicsBlurEffect>
#include <QStackedLayout>
#include <QtConcurrent>
#include <QFutureWatcher>

#include "Utils.h"

static int BACKUP_ID_ROLE = Qt::UserRole + 1;
static int BACKUP_DATE_ROLE = Qt::UserRole + 2;


// Dialog asking for backup name
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

BackupsOverlayModel::BackupsOverlayModel(QObject* parent): QStandardItemModel(parent) {
    setColumnCount(2);
    setHorizontalHeaderLabels({"Title", "Date"});
}

Qt::ItemFlags BackupsOverlayModel::flags(const QModelIndex &index) const {
    QModelIndex newIndex = index;
    if (index.column() != 0)
        newIndex = this->index(index.row(), 0);
    return QStandardItemModel::flags(newIndex);
}

QVariant BackupsOverlayModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid()) return QVariant();
    if (role == Qt::DisplayRole && index.column() == 1)
        role = BACKUP_DATE_ROLE;

    QModelIndex newIndex = index;
    if (index.column() != 0)
        newIndex = this->index(index.row(), 0);
    return QStandardItemModel::data(newIndex, role);
}

void BackupsOverlayModel::setBackupItems(std::vector<BackupMetadata>& backupItems) {
    QStandardItem* rootItem = invisibleRootItem();
    rootItem->removeRows(0, rootItem->rowCount());
    if (backupItems.empty()) return;
    std::sort(backupItems.begin(), backupItems.end(), [](const BackupMetadata& a, const BackupMetadata& b) {return a.epoch > b.epoch;});
    int autosaveCount = 0;
    int quicksaveCount = 0;
    QList<QStandardItem*> newItems;
    for (auto& backupItem: backupItems) {
        QString label = QString::fromStdString(backupItem.label);
        if (label.isEmpty()) {
            switch (backupItem.backupType) {
                case BackupType::AUTOSAVE:
                    label.push_back("< Auto Save");
                    label.push_back(QString::number(autosaveCount));
                    label.push_back(" >");
                    autosaveCount++;
                    break;

                case BackupType::QUICKSAVE:
                    label.push_back("< Quick Save");
                    label.push_back(QString::number(quicksaveCount));
                    label.push_back(" >");
                    quicksaveCount++;
                    break;

                default:
                    label = QString::fromStdString(backupItem.datetime);
                    break;
            }
        };
        QStandardItem* item = new QStandardItem(label);
        item->setColumnCount(columnCount());
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        item->setData(QVariant(QString::fromStdString(backupItem.id)), BACKUP_ID_ROLE);
        item->setData(QVariant(QString::fromStdString(backupItem.datetime)), BACKUP_DATE_ROLE);
        newItems.append(item);
    }
    if (!newItems.isEmpty())
        rootItem->appendRows(newItems);
}

// Overlay showin available backups
ManageBackupsOverlayWidget::ManageBackupsOverlayWidget(Controller* controller, QWidget* parent)
    : QWidget(parent), m_controller(controller)
{
    QFrame* shadowFrame = new QFrame(this);
    shadowFrame->setStyleSheet("background-color: rgba(0, 0, 0, 0.5);");

    QSplitter* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setContentsMargins(5, 5, 5, 5);

    QWidget* wrapWidget = new QWidget(splitter);

    QLabel* headerWidget = new QLabel("Backups", wrapWidget);
    headerWidget->setAlignment(Qt::AlignCenter);

    m_backupsView = new QTreeView(wrapWidget);
    m_backupsView->setSortingEnabled(true);
    m_backupsView->setAllColumnsShowFocus(true);
    m_backupsView->setAlternatingRowColors(true);
    m_backupsView->setIndentation(0);
    m_backupsView->setTextElideMode(Qt::ElideLeft);
    m_backupsView->sortByColumn(1, Qt::AscendingOrder);
    m_backupsView->setEditTriggers(
        QAbstractItemView::NoEditTriggers
    );
    m_backupsView->setSelectionMode(
        QAbstractItemView::ExtendedSelection
    );
    m_backupsView->setVerticalScrollMode(
        QAbstractItemView::ScrollPerPixel
    );

    m_backupsModel = new BackupsOverlayModel(m_backupsView);
    m_backupsView->setModel(m_backupsModel);

    QWidget* btnsWidget = new QWidget(wrapWidget);
    QPushButton* closeBackupBtn = new QPushButton("Close", btnsWidget);
    closeBackupBtn->setToolTip("Closes this overlay");
    QPushButton* createBackupBtn = new QPushButton("Create", btnsWidget);
    createBackupBtn->setToolTip("Create a backup of the current save");
    // TODO implement
    createBackupBtn->setVisible(false);
    QPushButton* deleteBackupsBtn = new QPushButton("Delete", btnsWidget);
    deleteBackupsBtn->setToolTip("Delete selected backups");

    QHBoxLayout* btnsLayout = new QHBoxLayout(btnsWidget);
    btnsLayout->setContentsMargins(0, 0, 0, 0);
    btnsLayout->setSpacing(10);
    btnsLayout->addWidget(closeBackupBtn, 0);
    btnsLayout->addStretch(1);
    btnsLayout->addWidget(createBackupBtn, 0);
    btnsLayout->addWidget(deleteBackupsBtn, 0);

    QVBoxLayout* wrapLayout = new QVBoxLayout(wrapWidget);
    wrapLayout->setContentsMargins(5, 5, 5, 5);
    wrapLayout->setSpacing(10);
    wrapLayout->addWidget(headerWidget, 0);
    wrapLayout->addWidget(m_backupsView, 1);
    wrapLayout->addWidget(btnsWidget, 0);

    ClickableFrame* fillupWidget = new ClickableFrame(this);

    splitter->addWidget(wrapWidget);
    splitter->addWidget(fillupWidget);
    splitter->setCollapsible(0, false);
    splitter->setCollapsible(1, false);
    splitter->setSizes({150, 200});
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 4);

    QStackedLayout* mainLayout = new QStackedLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(shadowFrame);
    mainLayout->addWidget(splitter);
    mainLayout->setStackingMode(QStackedLayout::StackAll);
    mainLayout->setCurrentWidget(splitter);

    this->setAttribute(Qt::WA_TranslucentBackground, true);
    splitter->setAttribute(Qt::WA_TranslucentBackground, true);
    fillupWidget->setAttribute(Qt::WA_TranslucentBackground, true);

    connect(fillupWidget, SIGNAL(clicked()), this, SIGNAL(hideRequested()));
    connect(closeBackupBtn, SIGNAL(clicked()), this, SIGNAL(hideRequested()));
    connect(deleteBackupsBtn, SIGNAL(clicked()), this, SLOT(onDeleteBackkups()));
}

void ManageBackupsOverlayWidget::showEvent(QShowEvent *event) {
    m_backupsView->resizeColumnToContents(0);
}

void ManageBackupsOverlayWidget::refresh() {
    auto watcher = new QFutureWatcher<std::vector<BackupMetadata>>(this);

    // Kick off background work
    QFuture<std::vector<BackupMetadata>> future = QtConcurrent::run([this]() {
        return m_controller->getBackupItems();  // must be thread-safe for reads
    });

    connect(watcher, &QFutureWatcher<std::vector<BackupMetadata>>::finished, this, [this, watcher]() {
        // Back on the GUI thread here
        auto items = watcher->result();
        m_backupsModel->setBackupItems(items);
        m_backupsView->resizeColumnToContents(0);
        watcher->deleteLater();
    });

    watcher->setFuture(future);
}

void ManageBackupsOverlayWidget::onDeleteBackkups() {
    std::vector<QString> backupIds;
    for (auto& index: m_backupsView->selectionModel()->selectedIndexes()) {
        QVariant backupId = index.data(BACKUP_ID_ROLE);
        if (!backupId.isValid() || backupId.isNull()) continue;
        backupIds.push_back(backupId.toString());
    }
    if (backupIds.empty()) return;
    m_controller->deleteBackupByIds(backupIds);
    refresh();
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
    emit showBackupsRequested();
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
