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

    QWidget* progressWidget = new QWidget(this);

    m_flameWidget = new FlameWidget(progressWidget);
    m_flameWidget->setVisible(false);

    m_statusLabel = new QLabel(progressWidget);
    m_statusLabel->setWordWrap(true);
    m_statusLabel->setVisible(false);

    m_progressBar = new QProgressBar(progressWidget);
    m_progressBar->setTextVisible(false);
    m_progressBar->setVisible(false);

    QVBoxLayout* statusLayout = new QVBoxLayout();
    statusLayout->setContentsMargins(0, 0, 0, 0);
    statusLayout->setSpacing(5);
    statusLayout->addStretch(1);
    statusLayout->addWidget(m_statusLabel, 0);
    statusLayout->addWidget(m_progressBar, 0);
    statusLayout->addStretch(1);

    QHBoxLayout* progressLayout = new QHBoxLayout(progressWidget);
    progressLayout->setContentsMargins(0, 0, 0, 0);
    progressLayout->setSpacing(10);
    progressLayout->addWidget(m_flameWidget, 0);
    progressLayout->addLayout(statusLayout, 1);

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
    layout->addWidget(progressWidget, 0);
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
        m_flameWidget->setVisible(false);
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
    m_flameWidget->setVisible(true);
    m_updatesModel->startUpdate();
}

void UpdateDialog::onReleasePageClick() {
    const auto info = m_updatesModel->getUpdateInfo();
    if (info) QDesktopServices::openUrl(info->releaseUrl);
}

void UpdateDialog::onDownloadProgress(qint64 received, qint64 total) {
    if (total <= 0) return;
    m_progressBar->setRange(0, 100);
    const int percent = static_cast<int>(received * 100 / total);
    m_progressBar->setValue(percent);
    m_statusLabel->setText(received == total ? "Installing..." : QString("Downloading... %1%").arg(percent));
}

void UpdateDialog::onInstallStart() {
    m_closeBtn->setEnabled(false);
    m_statusLabel->setText("Restarting...");
}

void UpdateDialog::onUpdateFail(const QString& message) {
    m_isUpdating = false;
    m_flameWidget->setVisible(false);
    m_progressBar->setVisible(false);
    m_statusLabel->setText(message + " You can download the update from the release page.");
    m_statusLabel->setVisible(true);
    m_updateBtn->setEnabled(true);
    m_closeBtn->setText("Later");
}
