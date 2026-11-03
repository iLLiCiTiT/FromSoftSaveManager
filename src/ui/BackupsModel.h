#pragma once

#include <QObject>
#include <QTimer>
#include <chrono>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "BackupTypes.h"
#include "BackupsDb.h"
#include "ConfigModel.h"
#include "../parse/Parse.h"


class AutoBackupHandler: public QObject {
    Q_OBJECT
signals:
    void autoBackupRequested(QString savePath, fssm::Game game);
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
    std::unordered_map<QString, std::chrono::system_clock::time_point> m_lastAutobackups;
    std::unordered_set<QString> m_changedSaves;
    std::unordered_map<QString, SaveFileItem> m_saveItemsBySaveId;
};

class BackupsModel: public QObject {
    Q_OBJECT
signals:
    void createBackupFinished(bool, BackupType);
    void loadBackupFinished(bool);
public:
    explicit BackupsModel(const std::vector<SaveFileItem>& saveItems, const ConfigAutobackup& autobackupConfig, BackupsDb* db, QObject* parent);

    void updateAutobackupConfig(const ConfigAutobackup& autobackupConfig);

    std::optional<BackupMetadata> createBackup(const QString& savePath, const fssm::Game& game, const BackupType& backupType);
    std::optional<BackupMetadata> createBackup(const QString& savePath, const fssm::Game& game, const BackupType& backupType, const QString& label);
    void createQuickSaveBackup(const QString& savePath, const fssm::Game& game);
    std::optional<BackupMetadata> createManualBackup(const QString& savePath, const fssm::Game& game, const QString& label);
    bool changeBackupLabel(const fssm::Game& game, const QString& backupId, const QString& label);

    std::vector<BackupMetadata> getBackupItems(const fssm::Game& game);
    bool restoreBackupSave(const QString& dstSavePath, const BackupMetadata& backupItem);
    bool restoreBackupById(const QString& dstSavePath, const fssm::Game& game, const QString& backupId);
    bool quickLoad(const QString& dstSavePath, const fssm::Game& game);
    void deleteBackupByIds(const fssm::Game& game, const std::vector<QString>& backupIds);
    void saveGameChanged(const QString& saveId);

private slots:
    void createAutoBackup(const QString& savePath, const fssm::Game& game);

private:
    AutoBackupHandler* m_autoBackupHandler;
    BackupsDb* m_db;
    int m_maxAutoBackups;
    bool writeSaveData(const QString& dstSavePath, const QByteArray& data);
    void cleanupAutoBackups(const fssm::Game& game);
};
