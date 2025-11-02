#include "SaveModel.h"

#include <QDateTime>
#include <QThread>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <thread>

#include "Utils.h"

using json = nlohmann::json;


BackupMetadata createBackupMetadata(
    const fsm::parse::Game& game,
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

std::string filenameByGame(fsm::parse::Game& game) {
    switch (game) {
        case fsm::parse::Game::DSR:
            return "DRAKS0005.sl2";
        case fsm::parse::Game::DS2_SOTFS:
            return "DS2SOFS0000.sl2";
        case fsm::parse::Game::DS3:
            return "DS30000.sl2";
        case fsm::parse::Game::Sekiro:
            return "S0000.sl2";
        case fsm::parse::Game::ER:
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

    fsm::parse::Game game = fsm::parse::Game::fromString(gameIt.value().get<std::string>());
    if (game == fsm::parse::Game::Unknown) return std::nullopt;

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


SaveModel::SaveModel(const QString& backupRoot, int maxBackups, QObject *parent)
    : QObject(parent),
    m_backupsRoot(backupRoot),
    m_maxAutoBackups(maxBackups)
{}

void SaveModel::createBackup(const QString& savePath, const fsm::parse::Game& game, const BackupType& backupType, const QString& label) {
    // Skip empty save path
    if (savePath.isEmpty()) return;
    std::string stdSavePath = savePath.toStdString();
    // Check if path to backup exists
    if (!std::filesystem::exists(stdSavePath)) return;

    std::string timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss").toStdString();
    std::string backupDir = m_backupsRoot.toStdString() + "\\" + game.toString() + "\\new_" + timestamp;
    std::string filename = getFilename(stdSavePath);
    backupDir = indexExistingPath(backupDir);
    std::string dstPath = backupDir + "\\" + filename;
    std::filesystem::create_directory(backupDir);
    std::filesystem::copy_file(stdSavePath, dstPath);
    std::string metadataPath = backupDir + "\\metadata.json";

    BackupMetadata metadata = createBackupMetadata(
        game,
        backupType,
        filename,
        label.toStdString(),
        backupDir
    );
    json jsonMetadata = backupMetadataToJson(metadata);
    std::ofstream o(metadataPath);
    o << jsonMetadata.dump(4) << std::endl;
    o.close();
    emit createBackupFinished(true, backupType);
}

void SaveModel::createBackup(const QString& savePath, const fsm::parse::Game& game, const BackupType& backupType) {
    createBackup(savePath, game, backupType, "");
}

void SaveModel::createAutoBackup(const QString& savePath, const fsm::parse::Game& game) {
    createBackup(savePath, game, BackupType::AUTOSAVE, "");
    cleanupAutoBackups(game);
}

void SaveModel::createQuickSaveBackup(const QString& savePath, const fsm::parse::Game& game) {
    createBackup(savePath, game, BackupType::QUICKSAVE, "");
}

void SaveModel::createManualBackup(const QString& savePath, const fsm::parse::Game& game, const QString& label) {
    if (label.isEmpty()) return;
    createBackup(savePath, game, BackupType::MANUAL, label);
}

std::vector<BackupMetadata> SaveModel::getBackupItems(const fsm::parse::Game &game) {
    std::vector<BackupMetadata> output;
    std::string gameBackupDir = m_backupsRoot.toStdString() + "\\" + game.toString();
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

bool SaveModel::restoreBackupSave(const QString& dstSavePath, const BackupMetadata &metadata) {
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
        return true;
    }

    return false;
}

bool SaveModel::restoreBackupById(const QString& dstSavePath, const fsm::parse::Game &game, const QString& backupId) {
    for (auto item: getBackupItems(game)) {
        if (item.id == backupId) {
            return restoreBackupSave(dstSavePath, item);
        };
    };
    return false;
}

bool SaveModel::quickLoad(const QString &dstSavePath, const fsm::parse::Game &game) {
    std::string gameBackupsDir = m_backupsRoot.toStdString() + "\\" + game.toString();
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

void SaveModel::deleteBackups(const std::vector<BackupMetadata>& backupItems) {
    for (auto& item: backupItems) {
        if (std::filesystem::exists(item.backupDir)) {
            std::filesystem::remove_all(item.backupDir);
        }
    }
}

void SaveModel::cleanupAutoBackups(const fsm::parse::Game& game) {
    if (m_maxAutoBackups < 1) return;

    std::vector<BackupMetadata> autosaveItems;
    for (auto item: getBackupItems(game)) {
        if (item.backupType == BackupType::AUTOSAVE) autosaveItems.push_back(item);
    }

    const auto epochComp = [](const BackupMetadata& lhs, const BackupMetadata& rhs) {
        return lhs.epoch < rhs.epoch;
    };
    std::sort(autosaveItems.begin(), autosaveItems.end(), epochComp);
    while (autosaveItems.size() > m_maxAutoBackups) {
        autosaveItems.pop_back();
    }
    deleteBackups(autosaveItems);
}

void SaveModel::deleteBackupByIds(const fsm::parse::Game& game, const std::vector<QString>& backupIds) {
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
