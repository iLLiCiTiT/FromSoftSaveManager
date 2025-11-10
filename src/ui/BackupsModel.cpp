#include "BackupsModel.h"

#include <QDateTime>
#include <QThread>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <thread>

#include "Utils.h"

using json = nlohmann::json;


// Helper functions
BackupMetadata createBackupMetadata(
    const fssm::Game& game,
    const BackupType& backupType,
    const std::string& filename,
    const std::string& label,
    const std::string& backupDir
) {
    std::vector<std::string> filenames;
    filenames.push_back(filename);
    std::string datetime = QDateTime::currentDateTimeUtc().toString(Qt::ISODate).toStdString();
    return BackupMetadata {
        .id =generateUUID(),
        .game = game,
        .backupType = backupType,
        .label = label,
        .filenames = filenames,
        .datetime = datetime,
        .epoch = std::time(nullptr),
        .backupDir = backupDir
    };
}

std::string filenameByGame(fssm::Game& game) {
    switch (game) {
        case fssm::Game::DSR:
            return "DRAKS0005.sl2";
        case fssm::Game::DS2_SOTFS:
            return "DS2SOFS0000.sl2";
        case fssm::Game::DS3:
            return "DS30000.sl2";
        case fssm::Game::Sekiro:
            return "S0000.sl2";
        case fssm::Game::ER:
            return "ER0000.sl2";
        default:
            return "";
    }
}

std::optional<BackupMetadata> backupMetadatafromJson(const std::filesystem::path& backupDir, const json& data)
{
    const auto idIt = data.find("id");
    const auto gameIt = data.find("game");
    if (
        idIt == data.end()
        || !idIt->is_string()
        || gameIt == data.end()
        || !gameIt->is_string()) return std::nullopt;

    fssm::Game game = fssm::Game::fromString(gameIt.value().get<std::string>());
    if (game == fssm::Game::Unknown) return std::nullopt;

    std::string filename = filenameByGame(game);
    if (filename.empty()) return std::nullopt;

    const auto backupTypeIt = data.find("backup_type");
    const auto dateTimeIt = data.find("datetime");
    const auto epochIt = data.find("epoch");
    const auto labelIt = data.find("label");

    std::string backupTypeS;
    if (backupTypeIt != data.end() && backupTypeIt->is_string())
        backupTypeS = backupTypeIt.value();
    BackupType backupType = backupTypeFromString(backupTypeS);

    // TODO handle missing or invalid values
    std::string datetime = "";
    if (backupTypeIt != data.end() && backupTypeIt->is_string())
        datetime = dateTimeIt.value();

    time_t epoch = 0;
    if (epochIt != data.end() && epochIt->is_number_integer())
        epoch = epochIt.value();

    if (datetime.empty() && epoch == 0) {
        std::filesystem::path savePath = backupDir;
        savePath /= filename;
        if (std::filesystem::exists(savePath)) {
            std::filesystem::file_time_type ftime = std::filesystem::last_write_time(savePath);
            auto stp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                ftime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now()
            );
            epoch = std::chrono::system_clock::to_time_t(stp);
        }
    }

    if (datetime.empty()) {
        QDateTime qdate = QDateTime::fromSecsSinceEpoch(epoch);
        datetime = qdate.toString(Qt::ISODate).toStdString();
    } else if (epoch == 0 ) {
        QDateTime qdate = QDateTime::fromString(QString::fromStdString(datetime), Qt::ISODate);
        epoch = qdate.toSecsSinceEpoch();
    }

    std::string label = "";
    if (labelIt != data.end() && labelIt->is_string())
        label = labelIt.value();

    if (label.empty() && backupType == BackupType::MANUAL) label = "NA";

    return BackupMetadata {
        .id = idIt.value(),
        .game = game,
        .backupType = backupType,
        .label = label,
        .filenames = {filename},
        .datetime = datetime,
        .epoch = epoch,
        .backupDir = backupDir.string()
    };
}

json backupMetadataToJson(const BackupMetadata& metadata)
{
    json jsonFilenames = json::array();
    for (const auto& filename: metadata.filenames) {
        jsonFilenames.push_back(filename);
    }
    json output = {
        {"id", metadata.id},
        {"game", metadata.game.toString()},
        {"backup_type", backupTypeToString(metadata.backupType)},
        {"filenames", jsonFilenames},
        {"datetime", metadata.datetime},
        {"epoch", metadata.epoch},
    };
    if (metadata.label.empty())
        output["label"] = nullptr;
    else
        output["label"] = metadata.label;
    return output;
}

// TODO move these to global utils
std::pair<std::string, std::string> splitPath(const std::string& path) {
    // Find filename start
    const auto is_sep = [](char c) { return c == '/' || c == '\\'; };
    std::size_t sep = std::string::npos;
    for (std::size_t i = path.size(); i > 0; --i) {
        if (is_sep(path[i - 1])) {
            sep = i - 1;
            break;
        }
    }
    if (sep == std::string::npos || sep == 0) return {path, ""};
    sep += 1;
    if (sep == path.size()) return {path, ""};

    return {path.substr(0, sep - 1), path.substr(sep)};
}

std::string getFilename(const std::string& path) {
    // Find filename start
    auto [dir, filename] = splitPath(path);
    return filename;
}

std::pair<std::string, std::string> splitExt(const std::string& path) {
    std::size_t dot = path.rfind('.');
    // No dot or dot is before the basename -> no extension
    if (dot == std::string::npos) {
        return {path, ""};
    }

    // Find filename start
    const auto is_sep = [](char c) { return c == '/' || c == '\\'; };
    std::size_t sep = std::string::npos;
    for (std::size_t i = path.size(); i > 0; --i) {
        if (is_sep(path[i - 1])) {
            sep = i - 1;
            break;
        }
    }
    const std::size_t start = (sep == std::string::npos) ? 0 : sep + 1;

    // No dot in filename or filename starts with a dot
    if (dot <= start) return {path, ""};

    return { path.substr(0, dot), path.substr(dot) };
}

std::string indexExistingPath(const std::string& path) {
    if (!std::filesystem::exists(path)) return path;
    std::string pathStart = path;
    std::string ext = "";
    if (!std::filesystem::is_directory(path)) {
        auto r = splitExt(path);
        pathStart = r.first;
        ext = r.second;
    }
    int idx = 1;
    while (true) {
        std::string newPath = pathStart + "_" + std::to_string(idx) + ext;
        if (!std::filesystem::exists(newPath)) return newPath;
        idx += 1;
    };
}

AutoBackupHandler::AutoBackupHandler(const std::vector<SaveFileItem>& saveItems, const ConfigAutobackup& autobackupConfig, QObject* parent): QObject(parent) {
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
    for (auto& item: saveItems) {
        m_saveItemsBySaveId[item.saveId] = item;
    }
}

void AutoBackupHandler::updateAutobackupConfig(const ConfigAutobackup& autobackupConfig) {
    m_enabled = autobackupConfig.enabled;
    m_maxAutoBackups = autobackupConfig.maxBackups;
    m_frequency = autobackupConfig.frequency;
    if (m_enabled && !m_timer->isActive()) {
        m_timer->start();
    }
}

void AutoBackupHandler::onTimer() {
    if (!m_enabled) {
        m_timer->stop();
        return;
    }
    std::unordered_set<QString> changedSaves = m_changedSaves;
    for (auto& saveId: changedSaves) {
        auto it = m_saveItemsBySaveId.find(saveId);
        if (it == m_saveItemsBySaveId.end()) continue;
        SaveFileItem& item = it->second;
        std::filesystem::path path = item.savePath.toStdString();
        if (!std::filesystem::exists(path)) continue;
        m_changedSaves.erase(saveId);
        emit autoBackupRequested(item.savePath, item.game);
    }
}


// Backup model implementation
BackupsModel::BackupsModel(const std::vector<SaveFileItem>& saveItems, const ConfigAutobackup& autobackupConfig, const QString& backupRoot, QObject *parent)
    : QObject(parent),
    m_backupsRoot(backupRoot),
    m_maxAutoBackups(autobackupConfig.maxBackups)
{
    m_autoBackupHandler = new AutoBackupHandler(saveItems, autobackupConfig, this);

    connect(m_autoBackupHandler, SIGNAL(autoBackupRequested(QString, fssm::Game)), this, SLOT(createAutoBackup(QString, fssm::Game)));
}

void BackupsModel::updateAutobackupConfig(const ConfigAutobackup& autobackupConfig) {
    m_maxAutoBackups = autobackupConfig.maxBackups;
    m_autoBackupHandler->updateAutobackupConfig(autobackupConfig);
}

std::optional<BackupMetadata> BackupsModel::createBackup(const QString& savePath, const fssm::Game& game, const BackupType& backupType, const QString& label) {
    // Skip empty save path
    if (savePath.isEmpty()) return std::nullopt;
    std::string stdSavePath = savePath.toStdString();
    // Check if path to backup exists
    if (!std::filesystem::exists(stdSavePath)) return std::nullopt;
    QDateTime curTime = QDateTime::currentDateTime();
    std::string timestamp = curTime.toString("yyyyMMdd_hhmmss").toStdString();
    std::string backupDir = getGameBackupDir(game) + "\\" + timestamp;
    std::string filename = getFilename(stdSavePath);
    backupDir = indexExistingPath(backupDir);
    std::string dstPath = backupDir + "\\" + filename;
    std::filesystem::create_directory(backupDir);
    std::filesystem::copy_file(stdSavePath, dstPath);
    std::string metadataPath = backupDir + "\\metadata.json";

    std::string labelStd = label.toStdString();
    if (labelStd.empty() && backupType == BackupType::MANUAL) {
        labelStd = curTime.toString("yyyy-MM-dd hh:mm:ss").toStdString();
    }

    BackupMetadata metadata = createBackupMetadata(
        game,
        backupType,
        filename,
        labelStd,
        backupDir
    );
    json jsonMetadata = backupMetadataToJson(metadata);
    std::ofstream o(metadataPath);
    o << jsonMetadata.dump(4) << std::endl;
    o.close();
    emit createBackupFinished(true, backupType);
    return metadata;
}

std::optional<BackupMetadata> BackupsModel::createBackup(const QString& savePath, const fssm::Game& game, const BackupType& backupType) {
    return createBackup(savePath, game, backupType, "");
}

void BackupsModel::createAutoBackup(const QString& savePath, const fssm::Game& game) {
    createBackup(savePath, game, BackupType::AUTOSAVE, "");
    cleanupAutoBackups(game);
}

void BackupsModel::createQuickSaveBackup(const QString& savePath, const fssm::Game& game) {
    createBackup(savePath, game, BackupType::QUICKSAVE, "");
}

std::optional<BackupMetadata> BackupsModel::createManualBackup(const QString& savePath, const fssm::Game& game, const QString& label) {
    return createBackup(savePath, game, BackupType::MANUAL, label);
}

void BackupsModel::saveBackupMetadata(const BackupMetadata& metadata) {
    // Check if path to backup exists
    std::string metadataPath = metadata.backupDir + "\\metadata.json";
    if (!std::filesystem::exists(metadataPath)) return;
    json jsonMetadata = backupMetadataToJson(metadata);
    std::ofstream o(metadataPath);
    o << jsonMetadata.dump(4) << std::endl;
    o.close();
}

bool BackupsModel::changeBackupLabel(const fssm::Game& game, const QString& backupId, const QString& label) {
    for (auto& item: getBackupItems(game)) {
        if (item.id == backupId) {
            item.label = label.toStdString();
            saveBackupMetadata(item);
            return true;
        }
    }
    return false;
}

std::string BackupsModel::getGameBackupDir(const fssm::Game& game) {
    return m_backupsRoot.toStdString() + "\\" + game.toString();
}

std::vector<BackupMetadata> BackupsModel::getBackupItems(const fssm::Game &game) {
    std::vector<BackupMetadata> output;
    std::string gameBackupDir = getGameBackupDir(game);
    if (!std::filesystem::exists(gameBackupDir)) return output;
    for (auto const& dir_entry : std::filesystem::directory_iterator{gameBackupDir}) {
        std::filesystem::path metadataPath = dir_entry.path();
        metadataPath /= "metadata.json";
        if (!std::filesystem::exists(metadataPath)) continue;

        std::ifstream ifs(metadataPath);
        json metadata = json::parse(ifs);
        ifs.close();
        auto metadateItem = backupMetadatafromJson(dir_entry.path(), metadata);
        if (metadateItem.has_value()) {
            output.push_back(metadateItem.value());
        }
    }
    return output;
}

bool BackupsModel::restoreBackupSave(const QString& dstSavePath, const BackupMetadata &metadata) {
    auto [dstDir, dstFilename] = splitPath(dstSavePath.toStdString());
    if (!std::filesystem::exists(dstDir)) {
        std::filesystem::create_directory(dstDir);
    }
    if (std::find(metadata.filenames.begin(), metadata.filenames.end(), dstFilename) == metadata.filenames.end()) {
        emit loadBackupFinished(false);
        return false;
    }
    std::string srcFilePath = metadata.backupDir + "\\" + dstFilename;

    std::ifstream ifs(srcFilePath, std::ios::binary);
    ifs.seekg(0,std::ios::end);
    std::streampos iLength = ifs.tellg();
    ifs.seekg(0,std::ios::beg);
    std::vector<char> buffer(iLength);
    ifs.read(&buffer[0], iLength);
    ifs.close();

    int attempts = 0;
    while (attempts < 10) {
        attempts++;
        std::ofstream dstFile(dstSavePath.toStdString(), std::ios::binary | std::ios::trunc);
        if (dstFile.fail()) {
            QThread::msleep(100);
            continue;
        }
        dstFile.write(buffer.data(), buffer.size());
        dstFile.close();
        emit loadBackupFinished(true);
        return true;
    }

    emit loadBackupFinished(false);
    return false;
}

bool BackupsModel::restoreBackupById(const QString& dstSavePath, const fssm::Game &game, const QString& backupId) {
    for (auto item: getBackupItems(game)) {
        if (item.id == backupId) {
            return restoreBackupSave(dstSavePath, item);
        };
    };
    return false;
}

bool BackupsModel::quickLoad(const QString &dstSavePath, const fssm::Game &game) {
    std::string gameBackupsDir = getGameBackupDir(game);
    std::optional<BackupMetadata> quicksaveItem = std::nullopt;
    time_t lastEpoch = 0;
    for (auto item: getBackupItems(game)) {
        if (item.backupType != BackupType::QUICKSAVE) continue;

        if (item.epoch > lastEpoch) {
            lastEpoch = item.epoch;
            quicksaveItem = item;
        };
    }

    if (!quicksaveItem.has_value()) return false;
    return restoreBackupSave(dstSavePath, quicksaveItem.value());
}

void BackupsModel::deleteBackups(const std::vector<BackupMetadata>& backupItems) {
    for (auto& item: backupItems) {
        if (std::filesystem::exists(item.backupDir)) {
            std::filesystem::remove_all(item.backupDir);
        }
    }
}

void BackupsModel::cleanupAutoBackups(const fssm::Game& game) {
    if (m_maxAutoBackups < 1) return;

    std::vector<BackupMetadata> autosaveItems;
    for (auto item: getBackupItems(game)) {
        if (item.backupType == BackupType::AUTOSAVE) autosaveItems.push_back(item);
    }

    const auto epochComp = [](const BackupMetadata& lhs, const BackupMetadata& rhs) {
        return lhs.epoch < rhs.epoch;
    };
    std::sort(autosaveItems.begin(), autosaveItems.end(), epochComp);
    for (int i = 0; i < m_maxAutoBackups; i++) {
        if (autosaveItems.empty()) return;
        autosaveItems.pop_back();
    }
    deleteBackups(autosaveItems);
}

void BackupsModel::deleteBackupByIds(const fssm::Game& game, const std::vector<QString>& backupIds) {
    std::unordered_set<std::string> backupIdsStd;
    for (auto& backupId: backupIds) {
        backupIdsStd.insert(backupId.toStdString());
    }
    std::vector<BackupMetadata> backupItems;
    for (auto item: getBackupItems(game)) {
        if (backupIdsStd.find(item.id) != backupIdsStd.end())
            backupItems.push_back(item);
    }
    deleteBackups(backupItems);
}

void BackupsModel::saveGameChanged(const QString& saveId) {
    m_autoBackupHandler->saveGameChanged(saveId);
}
