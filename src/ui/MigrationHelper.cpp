#include "MigrationHelper.h"

#include <QDateTime>
#include <QMessageBox>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <unordered_set>
#include <nlohmann/json.hpp>

#include "BackupsDb.h"
#include "../parse/Game.h"

using json = nlohmann::json;


static std::string filenameByGame(const fssm::Game& game) {
    switch (game) {
        case fssm::Game::DSR:       return "DRAKS0005.sl2";
        case fssm::Game::DS2_SOTFS: return "DS2SOFS0000.sl2";
        case fssm::Game::DS3:       return "DS30000.sl2";
        case fssm::Game::Sekiro:    return "S0000.sl2";
        case fssm::Game::ER:        return "ER0000.sl2";
        default:                    return "";
    }
}

struct LegacyEntry {
    std::filesystem::path backupDir;
    BackupMetadata meta;
};

static std::optional<LegacyEntry> parseLegacyEntry(const std::filesystem::path& backupDir) {
    std::filesystem::path metaPath = backupDir / "metadata.json";
    if (!std::filesystem::exists(metaPath)) return std::nullopt;

    json data;
    {
        std::ifstream ifs(metaPath);
        try {
            data = json::parse(ifs);
        } catch (const json::parse_error& ex) {
            std::cerr << "MigrationHelper: parse error in " << metaPath << ": " << ex.what() << "\n";
            return std::nullopt;
        }
    }

    const auto idIt   = data.find("id");
    const auto gameIt = data.find("game");
    if (idIt == data.end() || !idIt->is_string() ||
        gameIt == data.end() || !gameIt->is_string()) return std::nullopt;

    fssm::Game game = fssm::Game::fromString(gameIt->get<std::string>());
    if (game == fssm::Game::Unknown) return std::nullopt;

    std::string filename = filenameByGame(game);
    if (filename.empty()) return std::nullopt;

    const auto typeIt     = data.find("backup_type");
    const auto datetimeIt = data.find("datetime");
    const auto epochIt    = data.find("epoch");
    const auto labelIt    = data.find("label");

    std::string backupTypeStr;
    if (typeIt != data.end() && typeIt->is_string())
        backupTypeStr = typeIt->get<std::string>();
    BackupType backupType = backupTypeFromString(backupTypeStr);

    std::string datetime;
    if (datetimeIt != data.end() && datetimeIt->is_string())
        datetime = datetimeIt->get<std::string>();

    time_t epoch = 0;
    if (epochIt != data.end() && epochIt->is_number_integer())
        epoch = epochIt->get<time_t>();

    if (datetime.empty() && epoch == 0) {
        std::filesystem::path sl2 = backupDir / filename;
        if (std::filesystem::exists(sl2)) {
            auto ftime = std::filesystem::last_write_time(sl2);
            auto stp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                ftime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now()
            );
            epoch = std::chrono::system_clock::to_time_t(stp);
        }
    }
    if (datetime.empty()) {
        datetime = QDateTime::fromSecsSinceEpoch(epoch).toString(Qt::ISODate).toStdString();
    } else if (epoch == 0) {
        epoch = QDateTime::fromString(QString::fromStdString(datetime), Qt::ISODate).toSecsSinceEpoch();
    }

    std::string label;
    if (labelIt != data.end() && labelIt->is_string())
        label = labelIt->get<std::string>();
    if (label.empty() && backupType == BackupType::MANUAL)
        label = "NA";

    return LegacyEntry {
        backupDir,
        BackupMetadata {
            .id         = idIt->get<std::string>(),
            .game       = game,
            .backupType = backupType,
            .label      = label,
            .filename   = filename,
            .datetime   = datetime,
            .epoch      = epoch,
        }
    };
}

static std::vector<LegacyEntry> collectLegacyEntries(const QString& backupRoot) {
    std::vector<LegacyEntry> entries;
    std::filesystem::path root = backupRoot.toStdString();
    if (!std::filesystem::exists(root)) return entries;
    for (const auto& gameDir : std::filesystem::directory_iterator{root}) {
        if (!gameDir.is_directory()) continue;
        for (const auto& backupDir : std::filesystem::directory_iterator{gameDir.path()}) {
            if (!backupDir.is_directory()) continue;
            auto entry = parseLegacyEntry(backupDir.path());
            if (entry.has_value())
                entries.push_back(std::move(entry.value()));
        }
    }
    return entries;
}

bool hasLegacyBackups(const QString& backupRoot) {
    std::filesystem::path root = backupRoot.toStdString();
    if (!std::filesystem::exists(root)) return false;
    for (const auto& gameDir : std::filesystem::directory_iterator{root}) {
        if (!gameDir.is_directory()) continue;
        for (const auto& backupDir : std::filesystem::directory_iterator{gameDir.path()}) {
            if (!backupDir.is_directory()) continue;
            if (std::filesystem::exists(backupDir.path() / "metadata.json"))
                return true;
        }
    }
    return false;
}

bool migrateIfAgreed(const QString& backupRoot, BackupsDb* db, QWidget* parent) {
    auto entries = collectLegacyEntries(backupRoot);
    if (entries.empty()) return false;

    auto btn = QMessageBox::question(
        parent,
        "Migrate Backups",
        QString(
            "Found %1 backup(s) stored in the old format.\n\n"
            "Migrate them to the new SQLite database?\n"
            "The old backup directories will be removed after a successful migration."
        ).arg(entries.size()),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::Yes
    );
    if (btn != QMessageBox::Yes) return false;

    if (!db->beginTransaction()) {
        QMessageBox::warning(parent, "Migration Failed",
            "Could not begin database transaction.");
        return false;
    }

    std::vector<std::filesystem::path> migratedDirs;
    bool allOk = true;
    for (auto& entry : entries) {
        std::filesystem::path sl2Path = entry.backupDir / entry.meta.filename;
        if (!std::filesystem::exists(sl2Path)) {
            std::cerr << "MigrationHelper: save file missing, skipping: " << sl2Path << "\n";
            continue;
        }
        std::ifstream ifs(sl2Path, std::ios::binary);
        std::vector<char> buf(std::istreambuf_iterator<char>(ifs), {});
        QByteArray saveData(buf.data(), static_cast<qsizetype>(buf.size()));

        if (!db->insertBackup(entry.meta, saveData)) {
            allOk = false;
            break;
        }
        migratedDirs.push_back(entry.backupDir);
    }

    if (!allOk || !db->commit()) {
        db->rollback();
        QMessageBox::warning(parent, "Migration Failed",
            "An error occurred. No changes were made.");
        return false;
    }

    // Remove migrated backup subdirs; also remove parent game dirs if now empty
    std::unordered_set<std::string> gameDirs;
    for (auto& dir : migratedDirs) {
        gameDirs.insert(dir.parent_path().string());
        std::error_code ec;
        std::filesystem::remove_all(dir, ec);
        if (ec)
            std::cerr << "MigrationHelper: failed to remove " << dir << ": " << ec.message() << "\n";
    }
    for (auto& gameDir : gameDirs) {
        std::error_code ec;
        if (std::filesystem::is_empty(gameDir, ec) && !ec)
            std::filesystem::remove(gameDir, ec);
    }

    return true;
}
