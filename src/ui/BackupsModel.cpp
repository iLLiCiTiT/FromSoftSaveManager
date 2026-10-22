#include "BackupsModel.h"

#include <QDateTime>
#include <QThread>
#include <filesystem>
#include <fstream>
#include <iostream>

#include "Utils.h"


static std::string filenameByGame(const fssm::Game& game) {
    switch (game) {
        case fssm::Game::DSR:       return "DRAKS0005.sl2";
        case fssm::Game::DS2_SOTFS: return "DS2SOFS0000.sl2";
        case fssm::Game::DS3:       return "DS30000.sl2";
        case fssm::Game::Sekiro:    return "S0000.sl2";
        case fssm::Game::ER:        return "ER0000.sl2";
        default:                    return "";
    }
}

static BackupMetadata makeMetadata(const fssm::Game& game, const BackupType& backupType,
                                   const std::string& filename, const std::string& label,
                                   const std::string& folder) {
    return BackupMetadata {
        .id         = generateUUID(),
        .game       = game,
        .backupType = backupType,
        .label      = label,
        .filename   = filename,
        .datetime   = QDateTime::currentDateTimeUtc().toString(Qt::ISODate).toStdString(),
        .epoch      = std::time(nullptr),
        .folder     = folder,
    };
}


// --- AutoBackupHandler ---

AutoBackupHandler::AutoBackupHandler(const std::vector<SaveFileItem>& saveItems,
                                     const ConfigAutobackup& autobackupConfig,
                                     QObject* parent)
    : QObject(parent)
{
    m_timer = new QTimer(this);
    m_timer->setSingleShot(false);
    m_timer->setInterval(1000);
    connect(m_timer, SIGNAL(timeout()), this, SLOT(onTimer()));
    updatePaths(saveItems);
    updateAutobackupConfig(autobackupConfig);
}

void AutoBackupHandler::saveGameChanged(const QString& saveId) {
    m_changedSaves.insert(saveId);
}

void AutoBackupHandler::updatePaths(const std::vector<SaveFileItem>& saveItems) {
    m_saveItemsBySaveId.clear();
    for (auto& item : saveItems)
        m_saveItemsBySaveId[item.saveId] = item;
}

void AutoBackupHandler::updateAutobackupConfig(const ConfigAutobackup& autobackupConfig) {
    m_enabled        = autobackupConfig.enabled;
    m_maxAutoBackups = autobackupConfig.maxBackups;
    m_frequency      = autobackupConfig.frequency;
    if (m_enabled && !m_timer->isActive())
        m_timer->start();
}

void AutoBackupHandler::onTimer() {
    if (!m_enabled) {
        m_timer->stop();
        return;
    }
    std::unordered_set<QString> changedSaves = m_changedSaves;
    auto now = std::chrono::system_clock::now();

    for (auto& saveId : changedSaves) {
        if (m_frequency > 0) {
            auto it = m_lastAutobackups.find(saveId);
            if (it != m_lastAutobackups.end()) {
                auto diff = std::chrono::duration_cast<std::chrono::seconds>(now - it->second);
                if (diff.count() < m_frequency) continue;
            }
        }
        auto it = m_saveItemsBySaveId.find(saveId);
        if (it == m_saveItemsBySaveId.end()) continue;
        SaveFileItem& item = it->second;
        if (!std::filesystem::exists(item.savePath.toStdString())) continue;
        m_changedSaves.erase(saveId);
        m_lastAutobackups[saveId] = now;
        emit autoBackupRequested(item.savePath, item.game);
    }
}


// --- BackupsModel ---

BackupsModel::BackupsModel(const std::vector<SaveFileItem>& saveItems,
                           const ConfigAutobackup& autobackupConfig,
                           BackupsDb* db,
                           QObject* parent)
    : QObject(parent),
      m_db(db),
      m_maxAutoBackups(autobackupConfig.maxBackups)
{
    m_autoBackupHandler = new AutoBackupHandler(saveItems, autobackupConfig, this);
    connect(m_autoBackupHandler, SIGNAL(autoBackupRequested(QString, fssm::Game)),
            this, SLOT(createAutoBackup(QString, fssm::Game)));
}

void BackupsModel::updateAutobackupConfig(const ConfigAutobackup& autobackupConfig) {
    m_maxAutoBackups = autobackupConfig.maxBackups;
    m_autoBackupHandler->updateAutobackupConfig(autobackupConfig);
}

std::optional<BackupMetadata> BackupsModel::createBackup(const QString& savePath,
                                                         const fssm::Game& game,
                                                         const BackupType& backupType,
                                                         const QString& label,
                                                         const QString& folder) {
    if (savePath.isEmpty() || !std::filesystem::exists(savePath.toStdString()))
        return std::nullopt;

    std::ifstream ifs(savePath.toStdString(), std::ios::binary);
    if (!ifs) return std::nullopt;
    std::vector<char> buf(std::istreambuf_iterator<char>(ifs), {});
    QByteArray saveData(buf.data(), static_cast<qsizetype>(buf.size()));

    std::string filename = filenameByGame(game);
    std::string labelStd = label.toStdString();
    if (labelStd.empty() && backupType == BackupType::MANUAL)
        labelStd = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss").toStdString();

    BackupMetadata meta = makeMetadata(game, backupType, filename, labelStd, folder.toStdString());

    if (!m_db->insertBackup(meta, saveData)) return std::nullopt;

    emit createBackupFinished(true, backupType);
    return meta;
}

std::optional<BackupMetadata> BackupsModel::createBackup(const QString& savePath,
                                                         const fssm::Game& game,
                                                         const BackupType& backupType) {
    return createBackup(savePath, game, backupType, "");
}

void BackupsModel::createAutoBackup(const QString& savePath, const fssm::Game& game) {
    createBackup(savePath, game, BackupType::AUTOSAVE, "");
    cleanupAutoBackups(game);
}

void BackupsModel::createQuickSaveBackup(const QString& savePath, const fssm::Game& game) {
    createBackup(savePath, game, BackupType::QUICKSAVE, "");
}

std::optional<BackupMetadata> BackupsModel::createManualBackup(const QString& savePath,
                                                                const fssm::Game& game,
                                                                const QString& label,
                                                                const QString& folder) {
    return createBackup(savePath, game, BackupType::MANUAL, label, folder);
}

bool BackupsModel::changeBackupLabel(const fssm::Game& game, const QString& backupId, const QString& label) {
    return m_db->updateLabel(backupId, label, BackupType::MANUAL);
}

std::vector<BackupMetadata> BackupsModel::getBackupItems(const fssm::Game& game) {
    return m_db->getBackups(game);
}

bool BackupsModel::writeSaveData(const QString& dstSavePath, const QByteArray& data) {
    auto [dstDir, _] = [&]() -> std::pair<std::string, std::string> {
        std::string path = dstSavePath.toStdString();
        auto pos = path.find_last_of("/\\");
        if (pos == std::string::npos) return {path, ""};
        return {path.substr(0, pos), path.substr(pos + 1)};
    }();

    if (!std::filesystem::exists(dstDir))
        std::filesystem::create_directories(dstDir);

    int attempts = 0;
    while (attempts < 10) {
        attempts++;
        std::ofstream ofs(dstSavePath.toStdString(), std::ios::binary | std::ios::trunc);
        if (ofs.fail()) {
            QThread::msleep(100);
            continue;
        }
        ofs.write(data.constData(), data.size());
        ofs.close();
        emit loadBackupFinished(true);
        return true;
    }
    emit loadBackupFinished(false);
    return false;
}

bool BackupsModel::restoreBackupSave(const QString& dstSavePath, const BackupMetadata& metadata) {
    auto dataOpt = m_db->getSaveData(QString::fromStdString(metadata.id));
    if (!dataOpt.has_value()) {
        emit loadBackupFinished(false);
        return false;
    }
    return writeSaveData(dstSavePath, dataOpt.value());
}

bool BackupsModel::restoreBackupById(const QString& dstSavePath, const fssm::Game& game, const QString& backupId) {
    auto dataOpt = m_db->getSaveData(backupId);
    if (!dataOpt.has_value()) {
        emit loadBackupFinished(false);
        return false;
    }
    return writeSaveData(dstSavePath, dataOpt.value());
}

bool BackupsModel::quickLoad(const QString& dstSavePath, const fssm::Game& game) {
    std::optional<BackupMetadata> best;
    time_t bestEpoch = 0;
    for (auto& item : m_db->getBackups(game)) {
        if (item.backupType != BackupType::QUICKSAVE) continue;
        if (item.epoch > bestEpoch) {
            bestEpoch = item.epoch;
            best = item;
        }
    }
    if (!best.has_value()) return false;
    return restoreBackupSave(dstSavePath, best.value());
}

void BackupsModel::deleteBackupByIds(const fssm::Game& game, const std::vector<QString>& backupIds) {
    QStringList ids;
    for (auto& id : backupIds)
        ids.append(id);
    m_db->deleteBackups(ids);
}

std::vector<QString> BackupsModel::getFolders(const fssm::Game& game) {
    return m_db->getFolders(game);
}

bool BackupsModel::createFolder(const fssm::Game& game, const QString& name) {
    return m_db->createFolder(game, name);
}

bool BackupsModel::renameFolder(const fssm::Game& game, const QString& oldName, const QString& newName) {
    return m_db->renameFolder(game, oldName, newName);
}

bool BackupsModel::deleteFolder(const fssm::Game& game, const QString& name) {
    return m_db->deleteFolder(game, name);
}

bool BackupsModel::moveBackupsToFolder(const std::vector<QString>& backupIds, const QString& folder) {
    QStringList ids;
    for (auto& id : backupIds)
        ids.append(id);
    return m_db->moveBackups(ids, folder);
}

void BackupsModel::cleanupAutoBackups(const fssm::Game& game) {
    if (m_maxAutoBackups < 1) return;

    std::vector<BackupMetadata> autosaves;
    for (auto& item : m_db->getBackups(game)) {
        // Autosaves the user moved into a folder are kept
        if (item.backupType == BackupType::AUTOSAVE && item.folder.empty())
            autosaves.push_back(item);
    }

    std::sort(autosaves.begin(), autosaves.end(),
              [](const BackupMetadata& a, const BackupMetadata& b) { return a.epoch < b.epoch; });

    for (int i = 0; i < m_maxAutoBackups; ++i) {
        if (autosaves.empty()) return;
        autosaves.pop_back();
    }

    QStringList toDelete;
    for (auto& item : autosaves)
        toDelete.append(QString::fromStdString(item.id));
    m_db->deleteBackups(toDelete);
}

void BackupsModel::saveGameChanged(const QString& saveId) {
    m_autoBackupHandler->saveGameChanged(saveId);
}
