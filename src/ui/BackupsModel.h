#pragma once

#include <QObject>
#include <QTimer>
#include <unordered_set>

#include "ConfigModel.h"
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
    fssm::parse::Game game;
    BackupType backupType;
    std::string label;
    std::vector<std::string> filenames;
    std::string datetime;
    time_t epoch;
    std::string backupDir;
};

// Handle autobackup based on config and save items
class AutoBackupHandler: public QObject {
    Q_OBJECT
signals:
    void autoBackupRequested(QString savePath, fssm::parse::Game game);
public:
    explicit AutoBackupHandler(const std::vector<SaveFileItem>& saveItems, const ConfigAutobackup& autobackupConfig, QObject* parent);
    void saveGameChanged(const QString& saveId);
    void updatePaths(const std::vector<SaveFileItem>& saveItems);
    void updateAutobackupConfig(const ConfigAutobackup& autobackupConfig);
private slots:
    void onTimer();
private:
    QTimer* m_timer = nullptr;
    bool m_enabled = false;
    int m_maxAutoBackups = 0;
    int m_frequency = 0;
    std::unordered_set<QString> m_changedSaves;
    std::unordered_map<QString, SaveFileItem> m_saveItemsBySaveId;
};

class BackupsModel: public QObject {
    Q_OBJECT
signals:
    void createBackupFinished(bool, BackupType);
    void loadBackupFinished(bool);
public:
    explicit BackupsModel(const std::vector<SaveFileItem>& saveItems, const ConfigAutobackup& autobackupConfig, const QString& backupRoot, QObject *parent);

    void updateAutobackupConfig(const ConfigAutobackup& autobackupConfig);

    void createBackup(const QString& savePath, const fssm::parse::Game& game, const BackupType& backupType);
    void createBackup(const QString& savePath, const fssm::parse::Game& game, const BackupType& backupType, const QString& label);
    void createQuickSaveBackup(const QString& savePath, const fssm::parse::Game& game);
    void createManualBackup(const QString& savePath, const fssm::parse::Game& game, const QString& label);

    std::string getGameBackupDir(const fssm::parse::Game& game);
    std::vector<BackupMetadata> getBackupItems(const fsm::parse::Game& game);
    bool restoreBackupSave(const QString& dstSavePath, const BackupMetadata& backupItem);
    bool restoreBackupById(const QString& dstSavePath, const fsm::parse::Game &game, const QString& backupId);
    bool quickLoad(const QString& dstSavePath, const fsm::parse::Game &game);
    void deleteBackupByIds(const fsm::parse::Game& game, const std::vector<QString>& backupIds);
    void saveGameChanged(const QString& saveId);

private slots:
    void createAutoBackup(const QString& savePath, const fsm::parse::Game& game);

private:
    AutoBackupHandler* m_autoBackupHandler;
    QString m_backupsRoot;
    int m_maxAutoBackups;
    void cleanupAutoBackups(const fsm::parse::Game& game);
    void deleteBackups(const std::vector<BackupMetadata>& backupItems);
};
