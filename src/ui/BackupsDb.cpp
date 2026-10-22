#include "BackupsDb.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>
#include <iostream>


static const QString CONNECTION_NAME = "fssm_backups";

BackupsDb::BackupsDb(const QString& dbPath) {
    open(dbPath);
}

BackupsDb::~BackupsDb() {
    close();
}

bool BackupsDb::open(const QString& dbPath) {
    m_dbPath = dbPath;
    m_db = QSqlDatabase::addDatabase("QSQLITE", CONNECTION_NAME);
    m_db.setDatabaseName(dbPath);
    if (!m_db.open()) {
        std::cerr << "BackupsDb: failed to open: "
                  << m_db.lastError().text().toStdString() << "\n";
        m_valid = false;
        return false;
    }
    m_valid = createSchema();
    return m_valid;
}

void BackupsDb::close() {
    m_db.close();
    m_db = QSqlDatabase();
    QSqlDatabase::removeDatabase(CONNECTION_NAME);
    m_valid = false;
}

bool BackupsDb::reopen(const QString& dbPath) {
    close();
    return open(dbPath);
}

bool BackupsDb::createSchema() {
    QSqlQuery q(m_db);
    bool ok = q.exec(
        "CREATE TABLE IF NOT EXISTS backups ("
        "  id          TEXT    PRIMARY KEY,"
        "  game        TEXT    NOT NULL,"
        "  backup_type TEXT    NOT NULL,"
        "  filename    TEXT    NOT NULL,"
        "  datetime    TEXT    NOT NULL,"
        "  epoch       INTEGER NOT NULL,"
        "  label       TEXT,"
        "  save_data   BLOB    NOT NULL"
        ")"
    );
    if (!ok) {
        std::cerr << "BackupsDb: schema error: "
                  << q.lastError().text().toStdString() << "\n";
        return false;
    }

    // Databases created before folders existed don't have the 'folder' column.
    //  Adding a column with a default keeps them readable by older versions.
    bool hasFolder = false;
    if (q.exec("PRAGMA table_info(backups)")) {
        while (q.next()) {
            if (q.value(1).toString() == "folder") hasFolder = true;
        }
    }
    if (!hasFolder && !q.exec("ALTER TABLE backups ADD COLUMN folder TEXT NOT NULL DEFAULT ''")) {
        std::cerr << "BackupsDb: failed to add folder column: "
                  << q.lastError().text().toStdString() << "\n";
        return false;
    }

    // Folders are stored separately so empty folders survive
    ok = q.exec(
        "CREATE TABLE IF NOT EXISTS backup_folders ("
        "  game TEXT NOT NULL,"
        "  name TEXT NOT NULL,"
        "  PRIMARY KEY (game, name)"
        ")"
    );
    if (!ok)
        std::cerr << "BackupsDb: schema error: "
                  << q.lastError().text().toStdString() << "\n";
    return ok;
}

bool BackupsDb::beginTransaction() { return m_db.transaction(); }
bool BackupsDb::commit()          { return m_db.commit(); }
void BackupsDb::rollback()        { m_db.rollback(); }

bool BackupsDb::insertBackup(const BackupMetadata& meta, const QByteArray& saveData) {
    QSqlQuery q(m_db);
    q.prepare(
        "INSERT OR IGNORE INTO backups"
        " (id, game, backup_type, filename, datetime, epoch, label, save_data, folder)"
        " VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)"
    );
    q.addBindValue(QString::fromStdString(meta.id));
    q.addBindValue(QString::fromStdString(meta.game.toString()));
    q.addBindValue(QString::fromStdString(backupTypeToString(meta.backupType)));
    q.addBindValue(QString::fromStdString(meta.filename));
    q.addBindValue(QString::fromStdString(meta.datetime));
    q.addBindValue(static_cast<qlonglong>(meta.epoch));
    if (meta.label.empty())
        q.addBindValue(QVariant(QMetaType(QMetaType::QString)));
    else
        q.addBindValue(QString::fromStdString(meta.label));
    q.addBindValue(saveData);
    q.addBindValue(QString::fromStdString(meta.folder));
    if (!q.exec()) {
        std::cerr << "BackupsDb: insertBackup failed: "
                  << q.lastError().text().toStdString() << "\n";
        return false;
    }
    return true;
}

std::vector<BackupMetadata> BackupsDb::getBackups(const fssm::Game& game) const {
    std::vector<BackupMetadata> result;
    QSqlQuery q(m_db);
    q.prepare(
        "SELECT id, game, backup_type, filename, datetime, epoch, label, folder"
        " FROM backups WHERE game = ?"
    );
    q.addBindValue(QString::fromStdString(game.toString()));
    if (!q.exec()) {
        std::cerr << "BackupsDb: getBackups failed: "
                  << q.lastError().text().toStdString() << "\n";
        return result;
    }
    while (q.next()) {
        BackupMetadata meta;
        meta.id       = q.value(0).toString().toStdString();
        meta.game     = fssm::Game::fromString(q.value(1).toString().toStdString());
        meta.backupType = backupTypeFromString(q.value(2).toString().toStdString());
        meta.filename = q.value(3).toString().toStdString();
        meta.datetime = q.value(4).toString().toStdString();
        meta.epoch    = static_cast<time_t>(q.value(5).toLongLong());
        QVariant labelVar = q.value(6);
        meta.label = labelVar.isNull() ? "" : labelVar.toString().toStdString();
        if (meta.label.empty() && meta.backupType == BackupType::MANUAL)
            meta.label = "NA";
        meta.folder = q.value(7).toString().toStdString();
        result.push_back(std::move(meta));
    }
    return result;
}

std::optional<QByteArray> BackupsDb::getSaveData(const QString& backupId) const {
    QSqlQuery q(m_db);
    q.prepare("SELECT save_data FROM backups WHERE id = ?");
    q.addBindValue(backupId);
    if (!q.exec() || !q.next()) {
        std::cerr << "BackupsDb: getSaveData failed for id "
                  << backupId.toStdString() << "\n";
        return std::nullopt;
    }
    return q.value(0).toByteArray();
}

bool BackupsDb::updateLabel(const QString& backupId, const QString& label, BackupType type) {
    QSqlQuery q(m_db);
    q.prepare("UPDATE backups SET label = ?, backup_type = ? WHERE id = ?");
    if (label.isEmpty())
        q.addBindValue(QVariant(QMetaType(QMetaType::QString)));
    else
        q.addBindValue(label);
    q.addBindValue(QString::fromStdString(backupTypeToString(type)));
    q.addBindValue(backupId);
    if (!q.exec()) {
        std::cerr << "BackupsDb: updateLabel failed: "
                  << q.lastError().text().toStdString() << "\n";
        return false;
    }
    return q.numRowsAffected() > 0;
}

bool BackupsDb::deleteBackups(const QStringList& ids) {
    if (ids.isEmpty()) return true;
    QStringList placeholders;
    for (int i = 0; i < ids.size(); ++i)
        placeholders.append("?");
    QSqlQuery q(m_db);
    q.prepare("DELETE FROM backups WHERE id IN (" + placeholders.join(", ") + ")");
    for (const auto& id : ids)
        q.addBindValue(id);
    if (!q.exec()) {
        std::cerr << "BackupsDb: deleteBackups failed: "
                  << q.lastError().text().toStdString() << "\n";
        return false;
    }
    return true;
}

std::vector<QString> BackupsDb::getFolders(const fssm::Game& game) const {
    std::vector<QString> result;
    QSqlQuery q(m_db);
    // Folders referenced only by backups are listed too, in case the folders table is out of sync
    q.prepare(
        "SELECT name FROM backup_folders WHERE game = ?"
        " UNION SELECT folder FROM backups WHERE game = ? AND folder != ''"
        " ORDER BY 1 COLLATE NOCASE"
    );
    QString gameStr = QString::fromStdString(game.toString());
    q.addBindValue(gameStr);
    q.addBindValue(gameStr);
    if (!q.exec()) {
        std::cerr << "BackupsDb: getFolders failed: "
                  << q.lastError().text().toStdString() << "\n";
        return result;
    }
    while (q.next())
        result.push_back(q.value(0).toString());
    return result;
}

bool BackupsDb::createFolder(const fssm::Game& game, const QString& name) {
    if (name.isEmpty()) return false;
    QSqlQuery q(m_db);
    q.prepare("INSERT OR IGNORE INTO backup_folders (game, name) VALUES (?, ?)");
    q.addBindValue(QString::fromStdString(game.toString()));
    q.addBindValue(name);
    if (!q.exec()) {
        std::cerr << "BackupsDb: createFolder failed: "
                  << q.lastError().text().toStdString() << "\n";
        return false;
    }
    return true;
}

bool BackupsDb::renameFolder(const fssm::Game& game, const QString& oldName, const QString& newName) {
    if (oldName.isEmpty() || newName.isEmpty()) return false;
    if (oldName == newName) return true;
    QString gameStr = QString::fromStdString(game.toString());
    if (!m_db.transaction()) return false;
    QSqlQuery q(m_db);
    // Renaming onto an existing folder merges the two
    q.prepare("DELETE FROM backup_folders WHERE game = ? AND name = ?");
    q.addBindValue(gameStr);
    q.addBindValue(oldName);
    bool ok = q.exec();
    if (ok) {
        q.prepare("INSERT OR IGNORE INTO backup_folders (game, name) VALUES (?, ?)");
        q.addBindValue(gameStr);
        q.addBindValue(newName);
        ok = q.exec();
    }
    if (ok) {
        q.prepare("UPDATE backups SET folder = ? WHERE game = ? AND folder = ?");
        q.addBindValue(newName);
        q.addBindValue(gameStr);
        q.addBindValue(oldName);
        ok = q.exec();
    }
    if (!ok || !m_db.commit()) {
        std::cerr << "BackupsDb: renameFolder failed: "
                  << q.lastError().text().toStdString() << "\n";
        m_db.rollback();
        return false;
    }
    return true;
}

bool BackupsDb::deleteFolder(const fssm::Game& game, const QString& name) {
    if (name.isEmpty()) return false;
    QString gameStr = QString::fromStdString(game.toString());
    if (!m_db.transaction()) return false;
    QSqlQuery q(m_db);
    q.prepare("UPDATE backups SET folder = '' WHERE game = ? AND folder = ?");
    q.addBindValue(gameStr);
    q.addBindValue(name);
    bool ok = q.exec();
    if (ok) {
        q.prepare("DELETE FROM backup_folders WHERE game = ? AND name = ?");
        q.addBindValue(gameStr);
        q.addBindValue(name);
        ok = q.exec();
    }
    if (!ok || !m_db.commit()) {
        std::cerr << "BackupsDb: deleteFolder failed: "
                  << q.lastError().text().toStdString() << "\n";
        m_db.rollback();
        return false;
    }
    return true;
}

bool BackupsDb::moveBackups(const QStringList& ids, const QString& folder) {
    if (ids.isEmpty()) return true;
    QStringList placeholders;
    for (int i = 0; i < ids.size(); ++i)
        placeholders.append("?");
    QSqlQuery q(m_db);
    q.prepare("UPDATE backups SET folder = ? WHERE id IN (" + placeholders.join(", ") + ")");
    q.addBindValue(folder);
    for (const auto& id : ids)
        q.addBindValue(id);
    if (!q.exec()) {
        std::cerr << "BackupsDb: moveBackups failed: "
                  << q.lastError().text().toStdString() << "\n";
        return false;
    }
    return true;
}
