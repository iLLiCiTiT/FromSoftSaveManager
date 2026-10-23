#include "ManageBackupsWidget.h"

#include <QSplitter>
#include <QTreeView>
#include <QGraphicsBlurEffect>
#include <QStackedLayout>
#include <QPainter>
#include <QMenu>
#include <QDateTime>
#include <QInputDialog>
#include <QMimeData>

#include "Utils.h"

namespace {
const int BackupIdRole = Qt::UserRole + 1;
const int BackupDateRole = Qt::UserRole + 2;
const int BackupSortRole = Qt::UserRole + 3;
const int ItemKindRole = Qt::UserRole + 4;
const int FolderNameRole = Qt::UserRole + 5;

const int BackupKind = 0;
const int FolderKind = 1;

const QString BackupIdsMime = "application/x-fssm-backup-ids";
}


CloseButton::CloseButton(QWidget* parent): ClickableFrame(parent) {
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_pix = QPixmap(":/images/common/close.png");
}

void CloseButton::paintEvent(QPaintEvent* event) {
    QPainter painter = QPainter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    painter.setClipRect(event->rect());
    if (underMouse())
        painter.setBrush(QColor(255, 255, 255, 40));
    painter.drawEllipse(rect());
    painter.drawPixmap(rect(), m_pix.scaled(width(), height(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

BackupsOverlayModel::BackupsOverlayModel(Controller* controller, QObject* parent): QStandardItemModel(parent), m_controller(controller) {
    setColumnCount(2);
    setHorizontalHeaderLabels({"Title", "Date"});
}

Qt::ItemFlags BackupsOverlayModel::flags(const QModelIndex &index) const {
    if (!index.isValid())
        return QStandardItemModel::flags(index);
    if (index.column() != 0) {
        Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
        if (index.data(ItemKindRole).toInt() == FolderKind)
            flags |= Qt::ItemIsDropEnabled;
        else
            flags |= Qt::ItemIsDragEnabled;
        return flags;
    }
    return QStandardItemModel::flags(index);
}

QVariant BackupsOverlayModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid()) return QVariant();
    // Folder icon only in the first column
    if (role == Qt::DecorationRole && index.column() != 0)
        return QVariant();
    if (role == Qt::DisplayRole && index.column() == 1)
        role = BackupDateRole;
    else if (role == BackupSortRole && index.column() == 0)
        role = Qt::DisplayRole;

    QModelIndex newIndex = index;
    if (index.column() != 0)
        newIndex = this->index(index.row(), 0, index.parent());
    return QStandardItemModel::data(newIndex, role);
}

bool BackupsOverlayModel::setData(const QModelIndex& index, const QVariant& value, int role) {
    if (!index.isValid() || index.column() != 0) return false;
    if (role != Qt::EditRole) return false;

    QString newLabel = value.toString();
    auto* item = itemFromIndex(index);
    if (item->text() == newLabel) return false;

    if (item->data(ItemKindRole).toInt() == FolderKind) {
        newLabel = newLabel.trimmed();
        if (newLabel.isEmpty()) return false;
        if (!m_controller->renameBackupFolder(item->data(FolderNameRole).toString(), newLabel)) return false;
        // Renaming to an existing folder merges them, rebuild the tree
        emit refreshRequested();
        return true;
    }

    QString backupId = item->data(BackupIdRole).toString();
    if (!m_controller->changeBackupLabel(backupId, newLabel)) return false;

    item->setText(newLabel);
    return true;
}

QStringList BackupsOverlayModel::mimeTypes() const {
    return {BackupIdsMime};
}

QMimeData* BackupsOverlayModel::mimeData(const QModelIndexList &indexes) const {
    QStringList ids;
    for (auto& index: indexes) {
        if (index.column() != 0 || index.data(ItemKindRole).toInt() != BackupKind) continue;
        ids.append(index.data(BackupIdRole).toString());
    }
    if (ids.isEmpty()) return nullptr;
    QMimeData* data = new QMimeData();
    data->setData(BackupIdsMime, ids.join("\n").toUtf8());
    return data;
}

bool BackupsOverlayModel::canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const {
    return data->hasFormat(BackupIdsMime);
}

bool BackupsOverlayModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) {
    if (!data->hasFormat(BackupIdsMime)) return false;
    // Dropping on a backup means dropping into the folder where the backup is
    QModelIndex target = parent;
    if (target.isValid() && target.data(ItemKindRole).toInt() != FolderKind)
        target = target.parent();
    QString folder = target.isValid() ? target.data(FolderNameRole).toString() : "";

    std::vector<QString> ids;
    for (auto& id: QString::fromUtf8(data->data(BackupIdsMime)).split("\n", Qt::SkipEmptyParts))
        ids.push_back(id);
    if (ids.empty()) return false;
    m_controller->moveBackupsToFolder(ids, folder);
    // Rows are rebuilt on refresh. Returning 'false' prevents the view from removing dragged rows.
    QMetaObject::invokeMethod(this, "refreshRequested", Qt::QueuedConnection);
    return false;
}

QStandardItem* BackupsOverlayModel::createModelItem(BackupMetadata& backupItem, int& autosaveCount, int& quicksaveCount) {
    QString label = QString::fromStdString(backupItem.label);

    QString datetimeStd = QString::fromStdString(backupItem.datetime);
    if (datetimeStd.size() > 10 && datetimeStd[10] == QLatin1Char(' '))
        datetimeStd[10] = QLatin1Char('T');

    QDateTime dt = QDateTime::fromString(datetimeStd, Qt::ISODate);
    if (!dt.isValid()) {
        // Optional fallback if the string might include milliseconds
        dt = QDateTime::fromString(datetimeStd, Qt::ISODateWithMs);
    }
    QDateTime datetime = dt.toLocalTime();
    QString datetimeLabel = datetime.toString("yyyy.MM.dd HH:mm:ss");
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
                label = datetimeLabel;
                break;
        }
    }
    QStandardItem* item = new QStandardItem(label);
    item->setColumnCount(columnCount());
    item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsDragEnabled);
    item->setData(BackupKind, ItemKindRole);
    item->setData(QVariant(QString::fromStdString(backupItem.id)), BackupIdRole);
    item->setData(QVariant(datetimeLabel), BackupDateRole);
    item->setData(QVariant(datetime), BackupSortRole);
    return item;
}

QStandardItem* BackupsOverlayModel::createFolderItem(const QString& folder) {
    QStandardItem* item = new QStandardItem(QIcon(":/icons/folder_256x256.png"), folder);
    if (folder == m_defaultFolder) {
        QFont font = item->font();
        font.setBold(true);
        item->setFont(font);
        item->setToolTip("Default folder, new backups go here");
    }
    item->setColumnCount(columnCount());
    item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsDropEnabled);
    item->setData(FolderKind, ItemKindRole);
    item->setData(folder, FolderNameRole);
    m_folderItems[folder] = item;
    return item;
}

void BackupsOverlayModel::updateFolderCount(QStandardItem* folderItem) {
    int count = folderItem->rowCount();
    QString label = count == 1 ? QString("1 backup") : QString::number(count) + " backups";
    if (folderItem->data(FolderNameRole).toString() == m_defaultFolder)
        label += " (default)";
    folderItem->setData(label, BackupDateRole);
}

QModelIndex BackupsOverlayModel::folderIndex(const QString& folder) const {
    auto it = m_folderItems.find(folder);
    if (it == m_folderItems.end()) return {};
    return it->second->index();
}

QModelIndex BackupsOverlayModel::addBackupItem(BackupMetadata& backupItem) {
    QStandardItem* parentItem = invisibleRootItem();
    auto it = m_folderItems.find(QString::fromStdString(backupItem.folder));
    if (it != m_folderItems.end())
        parentItem = it->second;
    QStandardItem* item = createModelItem(backupItem, m_autosaveCount, m_quicksaveCount);
    parentItem->appendRow(item);
    if (parentItem != invisibleRootItem())
        updateFolderCount(parentItem);
    return item->index();
}

void BackupsOverlayModel::setBackupItems(std::vector<BackupMetadata>& backupItems, const std::vector<QString>& folders, const QString& defaultFolder) {
    m_defaultFolder = defaultFolder;
    m_autosaveCount = 0;
    m_quicksaveCount = 0;
    m_folderItems.clear();
    QStandardItem* rootItem = invisibleRootItem();
    rootItem->removeRows(0, rootItem->rowCount());

    QList<QStandardItem*> newItems;
    for (auto& folder: folders)
        newItems.append(createFolderItem(folder));

    std::sort(backupItems.begin(), backupItems.end(), [](const BackupMetadata& a, const BackupMetadata& b) {return a.epoch > b.epoch;});
    for (auto& backupItem: backupItems) {
        QStandardItem* item = createModelItem(backupItem, m_autosaveCount, m_quicksaveCount);
        auto it = m_folderItems.find(QString::fromStdString(backupItem.folder));
        if (it == m_folderItems.end()) {
            newItems.append(item);
        } else {
            it->second->appendRow(item);
        }
    }
    for (auto& [_, folderItem]: m_folderItems)
        updateFolderCount(folderItem);
    if (!newItems.isEmpty())
        rootItem->appendRows(newItems);
}

bool BackupsProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const {
    bool leftIsFolder = left.data(ItemKindRole).toInt() == FolderKind;
    bool rightIsFolder = right.data(ItemKindRole).toInt() == FolderKind;
    if (leftIsFolder != rightIsFolder) {
        // 'lessThan' result is reversed in descending order
        return sortOrder() == Qt::AscendingOrder ? leftIsFolder : rightIsFolder;
    }
    if (leftIsFolder) {
        int cmp = left.data(FolderNameRole).toString().compare(right.data(FolderNameRole).toString(), Qt::CaseInsensitive);
        return sortOrder() == Qt::AscendingOrder ? cmp < 0 : cmp > 0;
    }
    return QSortFilterProxyModel::lessThan(left, right);
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

    QWidget* headerWidget = new QWidget(wrapWidget);
    QLabel* headerLabel = new QLabel("Backups", headerWidget);
    headerLabel->setAlignment(Qt::AlignCenter);

    CloseButton* closeBackupBtn = new CloseButton(headerWidget);
    closeBackupBtn->setToolTip("Close overlay");

    QHBoxLayout* headerLayout = new QHBoxLayout(headerWidget);
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->addWidget(headerLabel, 1);
    headerLayout->addWidget(closeBackupBtn, 0);

    QWidget* btnsWidget = new QWidget(wrapWidget);

    QPixmap createIcon = QPixmap(":/images/common/save.png");
    QPushButton* createBackupBtn = new QPushButton(createIcon, "Save", btnsWidget);
    createBackupBtn->setToolTip("Create a backup of the current save\nin the selected folder, or in the default folder");
    // createBackupBtn->setStyleSheet("background: #38BA7C; color: #2B2D30;");

    QPushButton* createFolderBtn = new QPushButton(QIcon(":/icons/folder_256x256.png"), "Folder", btnsWidget);
    createFolderBtn->setToolTip("Create a folder to categorize backups.\nDrag backups onto a folder to move them.\nRight click a folder to make it the default for new backups.");

    QPixmap deleteIcon = QPixmap(":/images/common/delete.png");
    m_deleteBackupsBtn = new QPushButton(deleteIcon, "Delete", btnsWidget);
    m_deleteBackupsBtn->setObjectName("delete_btn");
    m_deleteBackupsBtn->setToolTip("Delete selected backups.\nDeleted folders keep their backups as unsorted.");
    m_deleteBackupsBtn->setEnabled(false);
    m_deleteBackupsBtn->setAutoDefault(false);
    m_deleteBackupsBtn->setDefault(false);

    QHBoxLayout* btnsLayout = new QHBoxLayout(btnsWidget);
    btnsLayout->setContentsMargins(0, 0, 0, 0);
    btnsLayout->setSpacing(10);
    btnsLayout->addWidget(createBackupBtn, 0);
    btnsLayout->addWidget(createFolderBtn, 0);
    btnsLayout->addWidget(m_deleteBackupsBtn, 0);
    btnsLayout->addStretch(1);

    m_backupsView = new QTreeView(wrapWidget);
    m_backupsView->setSortingEnabled(true);
    m_backupsView->setAllColumnsShowFocus(true);
    m_backupsView->setAlternatingRowColors(true);
    m_backupsView->setIndentation(12);
    m_backupsView->setEditTriggers(QTreeView::NoEditTriggers);
    m_backupsView->setTextElideMode(Qt::ElideLeft);
    m_backupsView->sortByColumn(1, Qt::DescendingOrder);
    m_backupsView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_backupsView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_backupsView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_backupsView->setDragEnabled(true);
    m_backupsView->setAcceptDrops(true);
    m_backupsView->setDropIndicatorShown(true);
    m_backupsView->setDragDropMode(QAbstractItemView::DragDrop);
    m_backupsView->setDefaultDropAction(Qt::MoveAction);

    m_backupsModel = new BackupsOverlayModel(m_controller, m_backupsView);

    m_proxyModel = new BackupsProxyModel(m_backupsView);
    m_proxyModel->setSourceModel(m_backupsModel);
    m_proxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);
    m_proxyModel->setSortRole(BackupSortRole);
    m_proxyModel->setDynamicSortFilter(true);

    m_backupsView->setModel(m_proxyModel);
    m_backupsView->setColumnWidth(0, 200);

    QVBoxLayout* wrapLayout = new QVBoxLayout(wrapWidget);
    wrapLayout->setContentsMargins(5, 5, 5, 5);
    wrapLayout->setSpacing(0);
    wrapLayout->addWidget(headerWidget, 0);
    wrapLayout->addWidget(btnsWidget, 0);
    wrapLayout->addSpacing(5);
    wrapLayout->addWidget(m_backupsView, 1);
    wrapLayout->addSpacing(10);

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
    connect(createBackupBtn, SIGNAL(clicked()), this, SLOT(onCreateBackup()));
    connect(createFolderBtn, SIGNAL(clicked()), this, SLOT(onCreateFolder()));
    connect(m_deleteBackupsBtn, SIGNAL(clicked()), this, SLOT(onDeleteBackups()));
    connect(m_backupsModel, SIGNAL(refreshRequested()), this, SLOT(refresh()));
    connect(m_backupsView, SIGNAL(doubleClicked(const QModelIndex &)), this, SLOT(onDoubleClick(const QModelIndex &)));
    connect(m_backupsView, SIGNAL(collapsed(const QModelIndex &)), this, SLOT(onCollapsed(const QModelIndex &)));
    connect(m_backupsView, SIGNAL(expanded(const QModelIndex &)), this, SLOT(onExpanded(const QModelIndex &)));
    connect(m_backupsView, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(onCustomContextMenu(const QPoint &)));
    connect(m_backupsView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this, SLOT(onSelectionChange(const QItemSelection&, const QItemSelection&)));
}

void ManageBackupsOverlayWidget::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_F2) {
        QModelIndex index = m_backupsView->currentIndex();
        if (index.isValid())
            m_backupsView->edit(m_proxyModel->index(index.row(), 0, index.parent()));
        event->accept();
    }
}

void ManageBackupsOverlayWidget::refresh() {
    auto items = m_controller->getBackupItems();
    auto folders = m_controller->getBackupFolders();
    m_backupsModel->setBackupItems(items, folders, m_controller->getDefaultBackupFolder());
    // Signals are blocked so restoring the state doesn't change remembered collapsed folders
    m_backupsView->blockSignals(true);
    for (auto& folder: folders) {
        QModelIndex index = m_proxyModel->mapFromSource(m_backupsModel->folderIndex(folder));
        m_backupsView->setExpanded(index, !m_collapsedFolders.count(folder));
    }
    m_backupsView->blockSignals(false);
}

void ManageBackupsOverlayWidget::onCollapsed(const QModelIndex &index) {
    if (index.data(ItemKindRole).toInt() == FolderKind)
        m_collapsedFolders.insert(index.data(FolderNameRole).toString());
}

void ManageBackupsOverlayWidget::onExpanded(const QModelIndex &index) {
    if (index.data(ItemKindRole).toInt() == FolderKind)
        m_collapsedFolders.erase(index.data(FolderNameRole).toString());
}

void ManageBackupsOverlayWidget::p_deleteItems(const QModelIndexList& indexes) {
    std::vector<QString> backupIds;
    std::vector<QString> folders;
    for (auto& index: indexes) {
        if (index.column() != 0) continue;
        if (index.data(ItemKindRole).toInt() == FolderKind) {
            folders.push_back(index.data(FolderNameRole).toString());
            continue;
        }
        QVariant backupId = index.data(BackupIdRole);
        if (!backupId.isValid() || backupId.isNull()) continue;
        backupIds.push_back(backupId.toString());
    }
    if (backupIds.empty() && folders.empty()) return;
    if (!backupIds.empty())
        m_controller->deleteBackupByIds(backupIds);
    // Backups in removed folders become unsorted, they're never deleted with the folder
    for (auto& folder: folders)
        m_controller->deleteBackupFolder(folder);
    refresh();
}

void ManageBackupsOverlayWidget::onDeleteBackups() {
    p_deleteItems(m_backupsView->selectionModel()->selectedRows());
}

std::optional<QString> ManageBackupsOverlayWidget::p_currentFolder() const {
    QModelIndex index = m_backupsView->currentIndex();
    if (!index.isValid() || !m_backupsView->selectionModel()->isSelected(index)) return std::nullopt;
    if (index.data(ItemKindRole).toInt() != FolderKind)
        index = index.parent();
    if (!index.isValid()) return std::nullopt;
    return index.data(FolderNameRole).toString();
}

void ManageBackupsOverlayWidget::onCreateBackup() {
    auto metadataOpt = m_controller->createManualBackup(p_currentFolder());
    if (!metadataOpt.has_value()) return;
    QModelIndex index = m_backupsModel->addBackupItem(metadataOpt.value());
    QModelIndex proxyIndex = m_proxyModel->mapFromSource(index);
    m_backupsView->setExpanded(proxyIndex.parent(), true);
    m_backupsView->setCurrentIndex(proxyIndex);
    m_backupsView->edit(proxyIndex);
}

void ManageBackupsOverlayWidget::onCreateFolder() {
    auto folders = m_controller->getBackupFolders();
    std::unordered_set<QString> existing(folders.begin(), folders.end());
    QString name = "New folder";
    for (int i = 2; existing.count(name); ++i)
        name = "New folder " + QString::number(i);
    if (!m_controller->createBackupFolder(name)) return;
    refresh();
    p_editFolder(name);
}

void ManageBackupsOverlayWidget::p_editFolder(const QString& folder) {
    QModelIndex index = m_proxyModel->mapFromSource(m_backupsModel->folderIndex(folder));
    if (!index.isValid()) return;
    m_backupsView->scrollTo(index);
    m_backupsView->setCurrentIndex(index);
    m_backupsView->edit(index);
}

void ManageBackupsOverlayWidget::p_moveBackups(const std::vector<QString>& backupIds, const QString& folder) {
    if (backupIds.empty()) return;
    m_controller->moveBackupsToFolder(backupIds, folder);
    m_collapsedFolders.erase(folder);
    refresh();
}

void ManageBackupsOverlayWidget::onSelectionChange(const QItemSelection& selected, const QItemSelection& deselected) {
     m_deleteBackupsBtn->setEnabled(m_backupsView->selectionModel()->hasSelection());
}

void ManageBackupsOverlayWidget::onDoubleClick(const QModelIndex& index) {
    // Double click on folder only expands/collapses it
    if (index.data(ItemKindRole).toInt() != BackupKind) return;
    QString backupId = index.data(BackupIdRole).toString();
    p_loadBackup(backupId);
}

void ManageBackupsOverlayWidget::p_loadBackup(const QString& backupId) {
    m_controller->restoreBackupById(backupId);
    emit hideRequested();
}

void ManageBackupsOverlayWidget::onCustomContextMenu(const QPoint& point) {
    QModelIndex posIndex = m_backupsView->indexAt(point);
    auto globPoint = m_backupsView->viewport()->mapToGlobal(point);
    if (posIndex.isValid())
        posIndex = posIndex.siblingAtColumn(0);

    // Empty space
    if (!posIndex.isValid()) {
        QMenu contextMenu(this);
        QAction* newFolderAction = contextMenu.addAction("New folder");
        if (contextMenu.exec(globPoint) == newFolderAction)
            onCreateFolder();
        return;
    }

    // Folder
    if (posIndex.data(ItemKindRole).toInt() == FolderKind) {
        QString folder = posIndex.data(FolderNameRole).toString();
        QMenu contextMenu(this);
        QAction* defaultAction = contextMenu.addAction("Default for new backups");
        defaultAction->setCheckable(true);
        defaultAction->setChecked(folder == m_controller->getDefaultBackupFolder());
        contextMenu.addSeparator();
        QAction* renameAction = contextMenu.addAction("Rename..");
        QAction* newFolderAction = contextMenu.addAction("New folder");
        QAction* removeAction = contextMenu.addAction("Remove folder (keep backups)");
        auto* triggered = contextMenu.exec(globPoint);
        if (triggered == defaultAction) {
            // Unchecking makes "Unsorted" the default again
            m_controller->setDefaultBackupFolder(defaultAction->isChecked() ? folder : "");
            refresh();
        } else if (triggered == renameAction) {
            m_backupsView->edit(posIndex);
        } else if (triggered == newFolderAction) {
            onCreateFolder();
        } else if (triggered == removeAction) {
            m_controller->deleteBackupFolder(folder);
            refresh();
        }
        return;
    }

    // Backups
    QString posBackupId = posIndex.data(BackupIdRole).toString();
    bool found = false;
    QList<QModelIndex> indexes;
    for (auto& index: m_backupsView->selectionModel()->selectedRows()) {
        if (index.data(ItemKindRole).toInt() != BackupKind) continue;
        if (posBackupId == index.data(BackupIdRole).toString()) {
            found = true;
        }
        indexes.append(index);
    }
    if (!found) indexes.clear();
    if (indexes.isEmpty()) indexes.append(posIndex);

    std::vector<QString> backupIds;
    for (auto& index: indexes)
        backupIds.push_back(index.data(BackupIdRole).toString());

    QMenu contextMenu(this);

    QAction* loadAction = contextMenu.addAction("Load..");
    loadAction->setEnabled(indexes.length() == 1);

    QAction* renameAction = contextMenu.addAction("Rename..");
    renameAction->setEnabled(indexes.length() == 1);

    // Move to folder
    QMenu* moveMenu = contextMenu.addMenu("Move to");
    QString currentFolder = posIndex.parent().isValid() ? posIndex.parent().data(FolderNameRole).toString() : "";
    std::unordered_map<QAction*, QString> moveActions;
    QAction* unsortedAction = moveMenu->addAction("< Unsorted >");
    moveActions[unsortedAction] = "";
    moveMenu->addSeparator();
    for (auto& folder: m_controller->getBackupFolders())
        moveActions[moveMenu->addAction(QIcon(":/icons/folder_256x256.png"), folder)] = folder;
    for (auto& [action, folder]: moveActions)
        action->setEnabled(indexes.length() > 1 || folder != currentFolder);
    moveMenu->addSeparator();
    QAction* moveNewAction = moveMenu->addAction("New folder..");

    QAction* deleteAction = contextMenu.addAction("Delete..");

    auto* triggered = contextMenu.exec(globPoint);
    if (triggered == nullptr) return;
    if (triggered == loadAction) {
        p_loadBackup(posBackupId);
    } else if (triggered == renameAction) {
        m_backupsView->edit(posIndex);
    } else if (triggered == deleteAction) {
        m_controller->deleteBackupByIds(backupIds);
        refresh();
    } else if (triggered == moveNewAction) {
        bool ok = false;
        QString name = QInputDialog::getText(this, "New Folder", "Folder name:", QLineEdit::Normal, "", &ok).trimmed();
        if (!ok || name.isEmpty()) return;
        m_controller->createBackupFolder(name);
        p_moveBackups(backupIds, name);
    } else {
        auto it = moveActions.find(triggered);
        if (it != moveActions.end())
            p_moveBackups(backupIds, it->second);
    }
}

// Common UI widget for all games
ManageBackupsButtonsWidget::ManageBackupsButtonsWidget(Controller* controller, QWidget* parent): QFrame(parent), m_controller(controller)
{
    m_hotkeysLabel = new QLabel(this);
    m_hotkeysLabel->setAlignment(Qt::AlignCenter);

    QWidget* btnsWidget = new QWidget(this);

    QPixmap showIcon = QPixmap(":/images/common/menu.png");
    QPushButton* showBackupsBtn = new QPushButton(showIcon, "Backups", btnsWidget);
    showBackupsBtn->setToolTip("Show available backups");

    QHBoxLayout* btnsLayout = new QHBoxLayout(btnsWidget);
    btnsLayout->setContentsMargins(0, 0, 0, 0);
    btnsLayout->addStretch(1);
    btnsLayout->addWidget(showBackupsBtn, 0);
    btnsLayout->addStretch(1);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_hotkeysLabel, 0);
    layout->addWidget(btnsWidget, 0);
    layout->addStretch(1);

    this->setAttribute(Qt::WA_TranslucentBackground, true);
    m_hotkeysLabel->setAttribute(Qt::WA_TranslucentBackground, true);
    btnsWidget->setAttribute(Qt::WA_TranslucentBackground, true);
    connect(showBackupsBtn, SIGNAL(clicked()), this, SLOT(onShowBackups()));
    connect(m_controller, SIGNAL(hotkeysConfigChanged()), this, SLOT(onHotkeysChange()));

    onHotkeysChange();
}

void ManageBackupsButtonsWidget::onShowBackups() {
    emit showBackupsRequested();
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
