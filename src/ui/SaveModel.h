#pragma once

#include <QObject>
#include <nlohmann/json.hpp>

#include "../parse/Parse.h"

enum class BackupType {
    QUICKSAVE,
    MANUAL,
    AUTOSAVE,
    UNKNOWN
};

static std::string backupTypeToString(const BackupType& backupType) {
    switch (backupType) {
        case BackupType::QUICKSAVE:
            return "quicksave";
        case BackupType::MANUAL:
            return "manualsave";
        case BackupType::AUTOSAVE:
            return "autosave";
        default:
            return "unknown";
    }
}

static BackupType backupTypeFromString(const std::string& backupType) {
    if (backupType == "quicksave") return BackupType::QUICKSAVE;
    if (backupType == "manualsave") return BackupType::MANUAL;
    if (backupType == "autosave") return BackupType::AUTOSAVE;
    return BackupType::UNKNOWN;
}


class BackupMetadata {
public:
    std::string id;
    fsm::parse::Game game;
    BackupType backupType;
    std::string label;
    std::vector<std::string> filenames;
    std::string datetime;
    time_t epoch;

    BackupMetadata(
        const std::string& id,
        const fsm::parse::Game& game,
        const BackupType& backupType,
        const std::string& label,
        std::vector<std::string> filenames,
        const std::string& datetime,
        time_t epoch
    );
    BackupMetadata(
        const fsm::parse::Game& game,
        const BackupType& backupType,
        const std::string& filename,
        const std::string& label
    );
    nlohmann::json toJson();
};

class SaveModel: public QObject {
    Q_OBJECT
signals:
    void createBackupFinished(bool, BackupType);
    void loadBackupFinished(bool);
public:
    explicit SaveModel(const QString& backupRoot, int maxBackups, QObject *parent);

    void setMaxAutoBackups(const int& maxAutoBackups) {m_maxAutoBackups = maxAutoBackups;}

    void createBackup(const QString& savePath, const fsm::parse::Game& game, const BackupType& backupType);
    void createBackup(const QString& savePath, const fsm::parse::Game& game, const BackupType& backupType, const QString& label);
    void createAutoBackup(const QString& savePath, const fsm::parse::Game& game);
    void createQuickSaveBackup(const QString& savePath, const fsm::parse::Game& game);
    void createManualBackup(const QString& savePath, const fsm::parse::Game& game, const QString& label);

    // TODO not a 'void'
    void getBackupItems(const fsm::parse::Game& game);
    void restoreBackupSave(const QString& dstSavePath, const std::string& backupDir, const BackupMetadata& metadata);
    void restoreBackupById(const QString& dstSavePath, const QString& backupId);
    void quickLoad(const QString& dstSavePath, const fsm::parse::Game &game);
    void deleteBackups(const std::vector<QString>& backupIds);

private:
    QString m_backupsRoot;
    int m_maxAutoBackups;
    void cleanupAutoBackups(const fsm::parse::Game& game);
};
