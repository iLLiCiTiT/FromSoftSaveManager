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

private:
    QSqlDatabase m_db;
    bool m_valid = false;
    bool createSchema();
};
