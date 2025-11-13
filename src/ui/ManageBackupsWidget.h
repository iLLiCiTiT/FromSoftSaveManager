#pragma once

#include <QLineEdit>
#include <QStandardItemModel>
#include <QTreeView>
#include <QSortFilterProxyModel>

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

class BackupsOverlayModel: public QStandardItemModel {
    Q_OBJECT
public:
    explicit BackupsOverlayModel(Controller* controller, QObject* parent = nullptr);
    QModelIndex addBackupItem(BackupMetadata& backupItem);
    void setBackupItems(std::vector<BackupMetadata>& backupItems);
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
private:
    Controller* m_controller = nullptr;
    int m_autosaveCount = 0;
    int m_quicksaveCount = 0;

    QStandardItem* createModelItem(BackupMetadata& backupItem, int& autosaveCount, int& quicksaveCount);
};

class ManageBackupsOverlayWidget: public QWidget {
    Q_OBJECT
signals:
    void hideRequested();
public:
    explicit ManageBackupsOverlayWidget(Controller* controller, QWidget* parent);
    void refresh();
protected:
    void keyPressEvent(QKeyEvent *event) override;
private slots:
    void onDeleteBackups();
    void onCreateBackup();
    void onOpenBackupDir();
    void onDoubleClick(const QModelIndex &index);
    void onCustomContextMenu(const QPoint &point);
    void onSelectionChange(const QItemSelection& selected, const QItemSelection& deselected);
private:
    Controller* m_controller = nullptr;
    BackupsOverlayModel* m_backupsModel = nullptr;
    QSortFilterProxyModel* m_proxyModel = nullptr;
    QTreeView* m_backupsView = nullptr;
    QPushButton* m_deleteBackupsBtn = nullptr;
    void p_loadBackup(const QString& backupId);
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
