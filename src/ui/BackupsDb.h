#pragma once

#include <QByteArray>
#include <QSqlDatabase>
#include <QString>
#include <optional>
#include <vector>

#include "BackupTypes.h"


class BackupsDb {
public:
    explicit BackupsDb(const QString& dbPath);
    ~BackupsDb();

    bool isValid() const { return m_valid; }

    bool beginTransaction();
    bool commit();
    void rollback();

    bool insertBackup(const BackupMetadata& meta, const QByteArray& saveData);
    std::vector<BackupMetadata> getBackups(const fssm::Game& game) const;
    std::optional<QByteArray> getSaveData(const QString& backupId) const;
    bool updateLabel(const QString& backupId, const QString& label, BackupType type);
    bool deleteBackups(const QStringList& ids);

    // Folders are per game. Backups with an empty folder are "Unsorted".
    std::vector<QString> getFolders(const fssm::Game& game) const;
    bool createFolder(const fssm::Game& game, const QString& name);
    bool renameFolder(const fssm::Game& game, const QString& oldName, const QString& newName);
    // Removes the folder, its backups are moved to "Unsorted" (never deleted)
    bool deleteFolder(const fssm::Game& game, const QString& name);
    bool moveBackups(const QStringList& ids, const QString& folder);
    // Folder where new backups of the game go, empty means "Unsorted"
    QString getDefaultFolder(const fssm::Game& game) const;
    // Empty 'name' unsets the default folder
    bool setDefaultFolder(const fssm::Game& game, const QString& name);

private:
    QSqlDatabase m_db;
    bool m_valid = false;
    bool createSchema();
};
