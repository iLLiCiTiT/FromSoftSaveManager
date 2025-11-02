#include "SaveModel.h"

#include <QDateTime>
#include <QThread>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <thread>

#include "Utils.h"

using json = nlohmann::json;


BackupMetadata::BackupMetadata(
    const std::string& id,
    const fsm::parse::Game& game,
    const BackupType& backupType,
    const std::string& label,
    std::vector<std::string> filenames,
    const std::string& datetime,
    const time_t epoch
) {
    this->id = id;
    this->game = game;
    this->backupType = backupType;
    this->label = label;
    this->filenames = filenames;
    this->datetime = datetime;
    this->epoch = epoch;
}

BackupMetadata::BackupMetadata(
    const fsm::parse::Game& game,
    const BackupType& backupType,
    const std::string& filename,
    const std::string& label
) {
    std::vector<std::string> filenames;
    filenames.push_back(filename);
    std::string datetime = QDateTime::currentDateTimeUtc().toString(Qt::ISODate).toStdString();

    this->id = generateUUID();
    this->game = game;
    this->backupType = backupType;
    this->label = label;
    this->filenames = filenames;
    this->datetime = datetime;
    this->epoch = std::time(0);
}

json BackupMetadata::toJson() {
    json jsonFilenames = json::array();
    for (const auto& filename: filenames) {
        jsonFilenames.push_back(filename);
    }
    json output = {
        {"id", id},
        {"game", game.toString()},
        {"backup_type", backupTypeToString(backupType)},
        {"filenames", jsonFilenames},
        {"datetime", datetime},
        {"epoch", epoch},
    };
    if (label.empty())
        output["label"] = nullptr;
    else
        output["label"] = label;
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
    std::cout<<dir<<std::endl;
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

    BackupMetadata metadata = BackupMetadata(
        game,
        backupType,
        filename,
        label.toStdString()
    );
    json jsonMetadata = metadata.toJson();
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

void SaveModel::getBackupItems(const fsm::parse::Game &game) {
    // TODO implement
}

void SaveModel::restoreBackupSave(const QString& dstSavePath, const std::string& backupDir, const BackupMetadata &metadata) {
    auto [dstDir, dstFilename] = splitPath(dstSavePath.toStdString());
    if (!std::filesystem::exists(dstDir)) {
        std::filesystem::create_directory(dstDir);
    }
    if (std::find(metadata.filenames.begin(), metadata.filenames.end(), dstFilename) == metadata.filenames.end()) {
        emit loadBackupFinished(false);
        return;
    }
    std::string srcFilePath = backupDir + "\\" + dstFilename;
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
        std::ofstream dstFile(dstFilename, std::ios::binary | std::ios::trunc);
        if (dstFile.fail()) {
            QThread::msleep(50);
            continue;
        }
        dstFile.write(buffer.data(), buffer.size());
        dstFile.close();
        break;
    }
}

void SaveModel::restoreBackupById(const QString& dstSavePath, const QString& backupId) {
    // TODO implement
}

void SaveModel::quickLoad(const QString &dstSavePath, const fsm::parse::Game &game) {
    std::string gameBackupsDir = m_backupsRoot.toStdString() + "\\" + game.toString();
    // TODO implement
}

void SaveModel::deleteBackups(const std::vector<QString>& backupIds) {
    // TODO implement
}

void SaveModel::cleanupAutoBackups(const fsm::parse::Game& game) {
    // TODO implement
}
