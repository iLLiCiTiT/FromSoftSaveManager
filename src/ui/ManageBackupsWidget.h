#pragma once

#include <QLineEdit>
#include <QStandardItemModel>
#include <QTreeView>
#include <QSortFilterProxyModel>
#include <unordered_map>
#include <unordered_set>

#include "Controller.h"
#include "Utils.h"


class CloseButton: public ClickableFrame {
    Q_OBJECT
public:
    explicit CloseButton(QWidget* parent = nullptr);
protected:
    QSize sizeHint() const override {
        QFontMetrics fm(font());
        return QSize(fm.height(), fm.height());
    }
    void enterEvent(QEnterEvent *event) override { repaint(); }
    void leaveEvent(QEvent *event) override{ repaint(); }
    void paintEvent(QPaintEvent* event) override;
private:
    QPixmap m_pix;
};

// Backups grouped by user folders. Folders are top level items, backups without a folder
//  ("Unsorted") are top level items too.
class BackupsOverlayModel: public QStandardItemModel {
    Q_OBJECT
signals:
    // Folders or backups changed in a way that requires full refresh
    void refreshRequested();
public:
    explicit BackupsOverlayModel(Controller* controller, QObject* parent = nullptr);
    QModelIndex addBackupItem(BackupMetadata& backupItem);
    void setBackupItems(std::vector<BackupMetadata>& backupItems, const std::vector<QString>& folders, const QString& defaultFolder);
    QModelIndex folderIndex(const QString& folder) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    QStringList mimeTypes() const override;
    QMimeData* mimeData(const QModelIndexList &indexes) const override;
    bool canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const override;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;
    Qt::DropActions supportedDropActions() const override { return Qt::MoveAction; }
    Qt::DropActions supportedDragActions() const override { return Qt::MoveAction; }
private:
    Controller* m_controller = nullptr;
    int m_autosaveCount = 0;
    int m_quicksaveCount = 0;
    std::unordered_map<QString, QStandardItem*> m_folderItems;
    QString m_defaultFolder;

    QStandardItem* createModelItem(BackupMetadata& backupItem, int& autosaveCount, int& quicksaveCount);
    QStandardItem* createFolderItem(const QString& folder);
    void updateFolderCount(QStandardItem* folderItem);
};

// Keeps folders above backups regardless of sort order
class BackupsProxyModel: public QSortFilterProxyModel {
    Q_OBJECT
public:
    explicit BackupsProxyModel(QObject* parent = nullptr): QSortFilterProxyModel(parent) {};
protected:
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;
};

class ManageBackupsOverlayWidget: public QWidget {
    Q_OBJECT
signals:
    void hideRequested();
public:
    explicit ManageBackupsOverlayWidget(Controller* controller, QWidget* parent);
public slots:
    void refresh();
protected:
    void keyPressEvent(QKeyEvent *event) override;
private slots:
    void onDeleteBackups();
    void onCreateBackup();
    void onCreateFolder();
    void onDoubleClick(const QModelIndex &index);
    void onCustomContextMenu(const QPoint &point);
    void onSelectionChange(const QItemSelection& selected, const QItemSelection& deselected);
    void onCollapsed(const QModelIndex &index);
    void onExpanded(const QModelIndex &index);
private:
    Controller* m_controller = nullptr;
    BackupsOverlayModel* m_backupsModel = nullptr;
    BackupsProxyModel* m_proxyModel = nullptr;
    QTreeView* m_backupsView = nullptr;
    QPushButton* m_deleteBackupsBtn = nullptr;
    // Folders are expanded by default, remember which were collapsed between refreshes
    std::unordered_set<QString> m_collapsedFolders;
    void p_loadBackup(const QString& backupId);
    void p_deleteItems(const QModelIndexList& indexes);
    void p_moveBackups(const std::vector<QString>& backupIds, const QString& folder);
    // Folder of current selection, nullopt means the default folder is used
    std::optional<QString> p_currentFolder() const;
    void p_editFolder(const QString& folder);
};

class ManageBackupsButtonsWidget: public QFrame {
    Q_OBJECT
signals:
    void showBackupsRequested();

public:
    explicit ManageBackupsButtonsWidget(Controller* m_controller, QWidget* parent);
private slots:
    void onShowBackups();
    void onHotkeysChange();
private:
    Controller* m_controller = nullptr;
    QLabel* m_hotkeysLabel = nullptr;
};
