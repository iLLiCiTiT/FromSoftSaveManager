#pragma once

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QStandardItemModel>
#include <QTreeView>
#include <QSortFilterProxyModel>

#include "Controller.h"

class BackupsOverlayModel: public QStandardItemModel {
    Q_OBJECT
public:
    explicit BackupsOverlayModel(QObject* parent = nullptr);
    void setBackupItems(std::vector<BackupMetadata>& backupItems);
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
};

class ManageBackupsOverlayWidget: public QWidget {
    Q_OBJECT
signals:
    void hideRequested();
public:
    explicit ManageBackupsOverlayWidget(Controller* controller, QWidget* parent);
    void refresh();
protected:
    void showEvent(QShowEvent *event) override;
private slots:
    void onDeleteBackkups();
private:
    Controller* m_controller = nullptr;
    BackupsOverlayModel* m_backupsModel = nullptr;
    QSortFilterProxyModel* m_proxyModel = nullptr;
    QTreeView* m_backupsView = nullptr;
};

class CreateBackupDialog: public QDialog {
    Q_OBJECT
public:
    explicit CreateBackupDialog(QWidget* parent);
    QString getBackupName();
private:
    QLineEdit* m_backupNameinput = nullptr;
};

class ManageBackupsButtonsWidget: public QFrame {
    Q_OBJECT
signals:
    void showBackupsRequested();

public:
    explicit ManageBackupsButtonsWidget(Controller* m_controller, QWidget* parent);
private slots:
    void onCreateBackup();
    void onShowBackups();
    void onOpenBackupDir();
    void onHotkeysChange();
private:
    Controller* m_controller = nullptr;
    QLabel* m_hotkeysLabel = nullptr;
};
