#pragma once

#include <QCryptographicHash>
#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QProcess>
#include <QUrl>
#include <optional>

struct UpdateInfo {
    QString version;
    QString releaseNotes;
    QUrl releaseUrl;
    QUrl assetUrl;
    // Lowercase hex SHA-256 of the asset as reported by GitHub
    QString assetSha256;
};

// Looks for newer GitHub release, downloads it and hands it over to the updater executable
class UpdatesModel: public QObject {
    Q_OBJECT
signals:
    void updateAvailable();
    void downloadProgressChanged(qint64 received, qint64 total);
    void installStarted();
    void updateFailed(const QString& message);

public:
    explicit UpdatesModel(QObject* parent = nullptr);

    void checkForUpdates();
    std::optional<UpdateInfo> getUpdateInfo() const;
    void startUpdate();
    void cancelUpdate();

private slots:
    void onCheckFinished();
    void onDownloadReadyRead();
    void onDownloadFinished();
    void onExtractFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onExtractError(QProcess::ProcessError error);

private:
    QString getWorkDir() const;
    QString getUpdaterPath() const;
    bool canInstall(QString& error) const;
    void launchUpdater();
    void failUpdate(const QString& message);

    QNetworkAccessManager m_network;
    QNetworkReply* m_downloadReply = nullptr;
    QFile m_downloadFile;
    QCryptographicHash m_downloadHash {QCryptographicHash::Sha256};
    QProcess* m_extractProcess = nullptr;
    std::optional<UpdateInfo> m_updateInfo;
};
