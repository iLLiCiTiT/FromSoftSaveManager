#pragma once

#include <QDialog>
#include <QLabel>
#include <QLineEdit>

#include "Controller.h"

// TODO implement
class ManageSavesOverlayWidget: public QWidget {
    Q_OBJECT
public:
    explicit ManageSavesOverlayWidget(QWidget* parent);

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
