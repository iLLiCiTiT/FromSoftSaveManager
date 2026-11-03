#include "UpdatesModel.h"

#include <QCoreApplication>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QStandardPaths>
#include <QTemporaryFile>
#include <QVersionNumber>
#include <iostream>

static const QString LATEST_RELEASE_URL = "https://api.github.com/repos/iLLiCiTiT/FromSoftSaveManager/releases/latest";
static const QString APP_EXE_NAME = "FromSoftSaveManager.exe";
static const QString UPDATER_EXE_NAME = "FromSoftSaveManagerUpdater.exe";
static const QString DOWNLOAD_NAME = "update.zip";

// The update must contain the same variant of the application as is running
#ifdef FSSM_USE_INV_IMAGES
static const QString ASSET_NAME = "FromSoftSaveManager.zip";
#else
static const QString ASSET_NAME = "FromSoftSaveManager-minified.zip";
#endif

static QNetworkRequest createRequest(const QUrl& url) {
    QNetworkRequest request(url);
    // GitHub API refuses requests without user agent
    request.setHeader(
        QNetworkRequest::UserAgentHeader,
        "FromSoftSaveManager/" + QCoreApplication::applicationVersion()
    );
    return request;
}

// Tags have 3 components and application version might have 4, and QVersionNumber
//   does not consider '0.6.2' and '0.6.2.0' to be equal unless normalized.
static std::optional<QVersionNumber> parseVersion(QString version) {
    if (version.startsWith('v')) version.remove(0, 1);
    qsizetype suffixIndex = 0;
    QVersionNumber parsed = QVersionNumber::fromString(version, &suffixIndex);
    if (parsed.isNull() || suffixIndex != version.size()) return std::nullopt;
    return parsed.normalized();
}

UpdatesModel::UpdatesModel(QObject* parent): QObject(parent) {
    // Remove leftovers of previous update
    QDir(getWorkDir()).removeRecursively();
}

QString UpdatesModel::getWorkDir() const {
    return QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation))
        .filePath("FromSoftSaveManagerUpdate");
}

QString UpdatesModel::getUpdaterPath() const {
    return QDir(QCoreApplication::applicationDirPath()).filePath(UPDATER_EXE_NAME);
}

void UpdatesModel::checkForUpdates() {
    QNetworkReply* reply = m_network.get(createRequest(QUrl(LATEST_RELEASE_URL)));
    connect(reply, SIGNAL(finished()), this, SLOT(onCheckFinished()));
}

std::optional<UpdateInfo> UpdatesModel::getUpdateInfo() const {
    return m_updateInfo;
}

void UpdatesModel::onCheckFinished() {
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    reply->deleteLater();
    // Failed check is not worth bothering the user, it is tried again on next start
    if (reply->error() != QNetworkReply::NoError) {
        std::cerr << "Update check failed: " << reply->errorString().toStdString() << std::endl;
        return;
    }

    const QJsonObject release = QJsonDocument::fromJson(reply->readAll()).object();
    const QString tag = release["tag_name"].toString();
    const auto latest = parseVersion(tag);
    const auto current = parseVersion(QCoreApplication::applicationVersion());
    if (!latest || !current || *latest <= *current) return;

    UpdateInfo info;
    info.version = tag;
    info.releaseNotes = release["body"].toString();
    info.releaseUrl = QUrl(release["html_url"].toString());
    for (const QJsonValue& value: release["assets"].toArray()) {
        const QJsonObject asset = value.toObject();
        if (asset["name"].toString() != ASSET_NAME) continue;
        info.assetUrl = QUrl(asset["browser_download_url"].toString());
        const QString digest = asset["digest"].toString();
        if (digest.startsWith("sha256:")) info.assetSha256 = digest.mid(7).toLower();
    }
    m_updateInfo = info;
    emit updateAvailable();
}

bool UpdatesModel::canInstall(QString& error) const {
    if (!m_updateInfo->assetUrl.isValid() || m_updateInfo->assetSha256.isEmpty()) {
        error = "The release does not contain a verifiable download for this version of the application.";
        return false;
    }
    if (!QFile::exists(getUpdaterPath())) {
        error = "The updater is missing from the application directory.";
        return false;
    }
    QTemporaryFile writeTest(QDir(QCoreApplication::applicationDirPath()).filePath("XXXXXX.tmp"));
    if (!writeTest.open()) {
        error = "The application directory is not writable.";
        return false;
    }
    return true;
}

void UpdatesModel::failUpdate(const QString& message) {
    if (m_downloadFile.isOpen()) m_downloadFile.close();
    QDir(getWorkDir()).removeRecursively();
    emit updateFailed(message);
}

void UpdatesModel::startUpdate() {
    if (!m_updateInfo || m_downloadReply != nullptr || m_extractProcess != nullptr) return;
    QString error;
    if (!canInstall(error)) {
        emit updateFailed(error);
        return;
    }

    QDir workDir(getWorkDir());
    workDir.removeRecursively();
    workDir.mkpath(".");
    m_downloadFile.setFileName(workDir.filePath(DOWNLOAD_NAME));
    if (!m_downloadFile.open(QIODevice::WriteOnly)) {
        failUpdate("Could not create a temporary file for the download.");
        return;
    }
    m_downloadHash.reset();

    m_downloadReply = m_network.get(createRequest(m_updateInfo->assetUrl));
    connect(m_downloadReply, SIGNAL(readyRead()), this, SLOT(onDownloadReadyRead()));
    connect(m_downloadReply, SIGNAL(downloadProgress(qint64, qint64)), this, SIGNAL(downloadProgressChanged(qint64, qint64)));
    connect(m_downloadReply, SIGNAL(finished()), this, SLOT(onDownloadFinished()));
}

void UpdatesModel::cancelUpdate() {
    if (m_downloadReply != nullptr) {
        // Disconnect so the abort is not reported as a failure
        QNetworkReply* reply = m_downloadReply;
        m_downloadReply = nullptr;
        reply->disconnect(this);
        reply->abort();
        reply->deleteLater();
    }
    if (m_extractProcess != nullptr) {
        QProcess* process = m_extractProcess;
        m_extractProcess = nullptr;
        process->disconnect(this);
        process->kill();
        process->waitForFinished();
        process->deleteLater();
    }
    if (m_downloadFile.isOpen()) m_downloadFile.close();
    QDir(getWorkDir()).removeRecursively();
}

void UpdatesModel::onDownloadReadyRead() {
    const QByteArray data = m_downloadReply->readAll();
    m_downloadHash.addData(data);
    m_downloadFile.write(data);
}

void UpdatesModel::onDownloadFinished() {
    onDownloadReadyRead();
    QNetworkReply* reply = m_downloadReply;
    m_downloadReply = nullptr;
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) {
        failUpdate("Download failed: " + reply->errorString());
        return;
    }

    const bool written = m_downloadFile.flush() && m_downloadFile.error() == QFile::NoError;
    m_downloadFile.close();
    if (!written) {
        failUpdate("Could not write the downloaded file.");
        return;
    }
    if (QString::fromLatin1(m_downloadHash.result().toHex()) != m_updateInfo->assetSha256) {
        failUpdate("The downloaded file is corrupted (checksum does not match).");
        return;
    }

    // Windows 10 and newer include tar which is able to read zip files. Use full path
    //   to not run a different 'tar.exe' that would be found using PATH.
    const QString systemRoot = qEnvironmentVariable("SystemRoot", "C:\\Windows");
    QDir workDir(getWorkDir());
    workDir.mkpath("extracted");
    m_extractProcess = new QProcess(this);
    connect(m_extractProcess, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(onExtractFinished(int, QProcess::ExitStatus)));
    connect(m_extractProcess, SIGNAL(errorOccurred(QProcess::ProcessError)), this, SLOT(onExtractError(QProcess::ProcessError)));
    m_extractProcess->start(
        QDir(systemRoot).filePath("System32/tar.exe"),
        {
            "-xf", QDir::toNativeSeparators(workDir.filePath(DOWNLOAD_NAME)),
            "-C", QDir::toNativeSeparators(workDir.filePath("extracted"))
        }
    );
}

void UpdatesModel::onExtractError(QProcess::ProcessError error) {
    // Crash is reported also by finished signal
    if (error != QProcess::FailedToStart || m_extractProcess == nullptr) return;
    m_extractProcess->deleteLater();
    m_extractProcess = nullptr;
    failUpdate("Could not extract the downloaded file.");
}

void UpdatesModel::onExtractFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    if (m_extractProcess == nullptr) return;
    m_extractProcess->deleteLater();
    m_extractProcess = nullptr;
    if (exitStatus != QProcess::NormalExit || exitCode != 0) {
        failUpdate("Could not extract the downloaded file.");
        return;
    }
    launchUpdater();
}

void UpdatesModel::launchUpdater() {
    // Zip files contain a single root directory with the application
    QDir extractedDir(QDir(getWorkDir()).filePath("extracted"));
    QString sourceDir = extractedDir.absolutePath();
    if (!extractedDir.exists(APP_EXE_NAME)) {
        const QStringList subdirs = extractedDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        if (subdirs.size() != 1 || !QDir(extractedDir.filePath(subdirs[0])).exists(APP_EXE_NAME)) {
            failUpdate("The downloaded file does not contain the application.");
            return;
        }
        sourceDir = extractedDir.filePath(subdirs[0]);
    }

    // The updater runs outside the application directory so it can replace every file in it
    const QString updaterCopy = QDir(getWorkDir()).filePath(UPDATER_EXE_NAME);
    if (!QFile::copy(getUpdaterPath(), updaterCopy)) {
        failUpdate("Could not prepare the updater.");
        return;
    }
    const bool started = QProcess::startDetached(
        updaterCopy,
        {
            QString::number(QCoreApplication::applicationPid()),
            QDir::toNativeSeparators(sourceDir),
            QDir::toNativeSeparators(QCoreApplication::applicationDirPath()),
            APP_EXE_NAME
        },
        getWorkDir()
    );
    if (!started) {
        failUpdate("Could not start the updater.");
        return;
    }
    emit installStarted();
    // 'quit' would ask windows to close first and the open modal dialog prevents that
    QCoreApplication::exit(0);
}
