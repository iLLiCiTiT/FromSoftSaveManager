#include "UpdateDialog.h"

#include <QCoreApplication>
#include <QDesktopServices>
#include <QHBoxLayout>
#include <QVBoxLayout>


UpdateDialog::UpdateDialog(UpdatesModel* updatesModel, QWidget* parent): QDialog(parent), m_updatesModel(updatesModel) {
    setWindowTitle("Update available");
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    resize(520, 360);

    m_titleLabel = new QLabel(this);
    m_titleLabel->setObjectName("settings_header");

    m_notesWidget = new QTextBrowser(this);
    m_notesWidget->setOpenExternalLinks(true);

    m_statusLabel = new QLabel(this);
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setVisible(false);

    m_progressBar = new QProgressBar(this);
    m_progressBar->setVisible(false);

    QWidget* btnsWidget = new QWidget(this);
    m_updateBtn = new QPushButton("Update", btnsWidget);
    m_updateBtn->setObjectName("save_btn");
    m_releasePageBtn = new QPushButton("Open release page", btnsWidget);
    m_closeBtn = new QPushButton("Later", btnsWidget);

    QHBoxLayout* btnsLayout = new QHBoxLayout(btnsWidget);
    btnsLayout->setContentsMargins(0, 0, 0, 0);
    btnsLayout->addStretch(1);
    btnsLayout->addWidget(m_updateBtn, 0);
    btnsLayout->addWidget(m_releasePageBtn, 0);
    btnsLayout->addWidget(m_closeBtn, 0);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(5);
    layout->addWidget(m_titleLabel, 0);
    layout->addWidget(m_notesWidget, 1);
    layout->addWidget(m_statusLabel, 0);
    layout->addWidget(m_progressBar, 0);
    layout->addWidget(btnsWidget, 0);

    connect(m_updateBtn, SIGNAL(clicked()), this, SLOT(onUpdateClick()));
    connect(m_releasePageBtn, SIGNAL(clicked()), this, SLOT(onReleasePageClick()));
    connect(m_closeBtn, SIGNAL(clicked()), this, SLOT(reject()));
    connect(m_updatesModel, SIGNAL(downloadProgressChanged(qint64, qint64)), this, SLOT(onDownloadProgress(qint64, qint64)));
    connect(m_updatesModel, SIGNAL(installStarted()), this, SLOT(onInstallStart()));
    connect(m_updatesModel, SIGNAL(updateFailed(QString)), this, SLOT(onUpdateFail(QString)));

    refresh();
}

void UpdateDialog::refresh() {
    const auto info = m_updatesModel->getUpdateInfo();
    if (!info) return;
    m_titleLabel->setText(
        "Version " + info->version + " is available (current version is "
        + QCoreApplication::applicationVersion() + ")"
    );
    m_notesWidget->setMarkdown(info->releaseNotes);
    m_notesWidget->setVisible(!info->releaseNotes.isEmpty());
}

void UpdateDialog::reject() {
    if (m_isUpdating) {
        m_updatesModel->cancelUpdate();
        m_isUpdating = false;
        m_statusLabel->setVisible(false);
        m_progressBar->setVisible(false);
        m_updateBtn->setEnabled(true);
        m_closeBtn->setText("Later");
        return;
    }
    QDialog::reject();
}

void UpdateDialog::onUpdateClick() {
    m_isUpdating = true;
    m_updateBtn->setEnabled(false);
    m_closeBtn->setText("Cancel");
    m_statusLabel->setText("Downloading...");
    m_statusLabel->setVisible(true);
    m_progressBar->setRange(0, 0);
    m_progressBar->setVisible(true);
    m_updatesModel->startUpdate();
}

void UpdateDialog::onReleasePageClick() {
    const auto info = m_updatesModel->getUpdateInfo();
    if (info) QDesktopServices::openUrl(info->releaseUrl);
}

void UpdateDialog::onDownloadProgress(qint64 received, qint64 total) {
    if (total <= 0) return;
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(static_cast<int>(received * 100 / total));
    if (received == total) m_statusLabel->setText("Installing...");
}

void UpdateDialog::onInstallStart() {
    m_closeBtn->setEnabled(false);
    m_statusLabel->setText("Restarting...");
}

void UpdateDialog::onUpdateFail(const QString& message) {
    m_isUpdating = false;
    m_progressBar->setVisible(false);
    m_statusLabel->setText(message + " You can download the update from the release page.");
    m_statusLabel->setVisible(true);
    m_updateBtn->setEnabled(true);
    m_closeBtn->setText("Later");
}
