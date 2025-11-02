#pragma once

#include <QObject>
#include <nlohmann/json.hpp>

#include "../parse/Parse.h"

enum class BackupType {
    QUICKSAVE,
    AUTOSAVE,
    MANUAL
};

static std::string backupTypeToString(const BackupType& backupType) {
    switch (backupType) {
        case BackupType::QUICKSAVE:
            return "quicksave";
        case BackupType::AUTOSAVE:
            return "autosave";
        default:
            return "manualsave";
    }
}

static BackupType backupTypeFromString(const std::string& backupType) {
    if (backupType == "quicksave") return BackupType::QUICKSAVE;
    if (backupType == "autosave") return BackupType::AUTOSAVE;
    return BackupType::MANUAL;
}


struct BackupMetadata {
    std::string id;
    fsm::parse::Game game;
    BackupType backupType;
    std::string label;
    std::vector<std::string> filenames;
    std::string datetime;
    time_t epoch;
    std::string backupDir;
};

class BackupsModel: public QObject {
    Q_OBJECT
signals:
    void createBackupFinished(bool, BackupType);
    void loadBackupFinished(bool);
public:
    explicit BackupsModel(const QString& backupRoot, int maxBackups, QObject *parent);

    void setMaxAutoBackups(const int& maxAutoBackups) {m_maxAutoBackups = maxAutoBackups;}

    void createBackup(const QString& savePath, const fsm::parse::Game& game, const BackupType& backupType);
    void createBackup(const QString& savePath, const fsm::parse::Game& game, const BackupType& backupType, const QString& label);
    void createAutoBackup(const QString& savePath, const fsm::parse::Game& game);
    void createQuickSaveBackup(const QString& savePath, const fsm::parse::Game& game);
    void createManualBackup(const QString& savePath, const fsm::parse::Game& game, const QString& label);

    std::vector<BackupMetadata> getBackupItems(const fsm::parse::Game& game);
    bool restoreBackupSave(const QString& dstSavePath, const BackupMetadata& backupItem);
    bool restoreBackupById(const QString& dstSavePath, const fsm::parse::Game &game, const QString& backupId);
    bool quickLoad(const QString& dstSavePath, const fsm::parse::Game &game);
    void deleteBackupByIds(const fsm::parse::Game& game, const std::vector<QString>& backupIds);
    void deleteBackups(const std::vector<BackupMetadata>& backupItems);

private:
    QString m_backupsRoot;
    int m_maxAutoBackups;
    void cleanupAutoBackups(const fsm::parse::Game& game);
};
