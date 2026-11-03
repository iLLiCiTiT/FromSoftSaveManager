#pragma once

#include <QDialog>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QTextBrowser>

#include "UpdatesModel.h"

class UpdateDialog: public QDialog {
    Q_OBJECT
public:
    explicit UpdateDialog(UpdatesModel* updatesModel, QWidget* parent);
    void refresh();

public slots:
    void reject() override;

private slots:
    void onUpdateClick();
    void onReleasePageClick();
    void onDownloadProgress(qint64 received, qint64 total);
    void onInstallStart();
    void onUpdateFail(const QString& message);

private:
    UpdatesModel* m_updatesModel;
    bool m_isUpdating = false;
    QLabel* m_titleLabel;
    QTextBrowser* m_notesWidget;
    QLabel* m_statusLabel;
    QProgressBar* m_progressBar;
    QPushButton* m_updateBtn;
    QPushButton* m_releasePageBtn;
    QPushButton* m_closeBtn;
};
