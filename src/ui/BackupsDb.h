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
    QString dbPath() const { return m_dbPath; }
    // Close the current database and open (or create) the one at 'dbPath'
    bool reopen(const QString& dbPath);

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

private:
    QSqlDatabase m_db;
    QString m_dbPath;
    bool m_valid = false;
    bool open(const QString& dbPath);
    void close();
    bool createSchema();
};
