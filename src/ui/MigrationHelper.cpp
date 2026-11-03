#include "MigrationHelper.h"

#include <QDateTime>
#include <QDesktopServices>
#include <QDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QLocale>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
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
    uintmax_t size = 0;
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
        },
    };
}

struct LegacyScan {
    std::vector<LegacyEntry> entries;
    // Backups that can't be migrated and are left untouched
    QStringList skipped;
};

// Short label of a backup directory, e.g. 'DS3/<backup id>'
static QString backupDirLabel(const std::filesystem::path& backupDir) {
    return QString::fromStdWString((backupDir.parent_path().filename() / backupDir.filename()).generic_wstring());
}

static LegacyScan scanLegacyBackups(const QString& backupRoot) {
    LegacyScan scan;
    std::filesystem::path root = backupRoot.toStdWString();
    if (!std::filesystem::exists(root)) return scan;
    for (const auto& gameDir : std::filesystem::directory_iterator{root}) {
        if (!gameDir.is_directory()) continue;
        for (const auto& backupDir : std::filesystem::directory_iterator{gameDir.path()}) {
            if (!backupDir.is_directory()) continue;
            if (!std::filesystem::exists(backupDir.path() / "metadata.json")) continue;

            auto entry = parseLegacyEntry(backupDir.path());
            if (!entry.has_value()) {
                scan.skipped.append(backupDirLabel(backupDir.path()) + ": invalid metadata");
                continue;
            }
            std::error_code ec;
            uintmax_t size = std::filesystem::file_size(backupDir.path() / entry->meta.filename, ec);
            if (ec || size == 0) {
                scan.skipped.append(backupDirLabel(backupDir.path()) + ": save file is missing or empty");
                continue;
            }
            entry->size = size;
            scan.entries.push_back(std::move(entry.value()));
        }
    }
    return scan;
}

bool hasLegacyBackups(const QString& backupRoot) {
    std::filesystem::path root = backupRoot.toStdWString();
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

namespace {
class MigrationDialog: public QDialog {
public:
    MigrationDialog(const QString& backupRoot, LegacyScan scan, BackupsDb* db, QWidget* parent);

    bool isMigrated() const { return m_state == State::Finished && m_migrated > 0; }

protected:
    void reject() override;

private:
    enum class State { Confirm, Copying, Removing, Finished };

    void startMigration();
    void copyNextEntry();
    void removeNextDir();
    void showFailure(const QString& reason);
    void showResult();
    void showProblems(const QStringList& problems);

    QString m_backupRoot;
    std::vector<LegacyEntry> m_entries;
    QStringList m_skipped;
    QStringList m_notRemoved;
    BackupsDb* m_db;
    State m_state = State::Confirm;
    size_t m_nextIdx = 0;
    int m_migrated = 0;
    std::vector<std::filesystem::path> m_migratedDirs;

    QLabel* m_titleLabel;
    QLabel* m_messageLabel;
    QLabel* m_gamesLabel;
    QProgressBar* m_progressBar;
    QPlainTextEdit* m_problemsInput;
    QPushButton* m_openDirBtn;
    QPushButton* m_cancelBtn;
    QPushButton* m_confirmBtn;
};

MigrationDialog::MigrationDialog(const QString& backupRoot, LegacyScan scan, BackupsDb* db, QWidget* parent)
    : QDialog(parent), m_backupRoot(backupRoot), m_entries(std::move(scan.entries)), m_skipped(std::move(scan.skipped)), m_db(db)
{
    setWindowTitle("Migrate Backups");
    setMinimumWidth(460);

    m_titleLabel = new QLabel("Move your backups to the new storage", this);
    m_titleLabel->setStyleSheet("font-size: 12pt; font-weight: bold;");

    m_messageLabel = new QLabel(this);
    m_messageLabel->setWordWrap(true);

    m_gamesLabel = new QLabel(this);

    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, static_cast<int>(m_entries.size()));
    m_progressBar->setTextVisible(false);
    m_progressBar->setVisible(false);

    m_problemsInput = new QPlainTextEdit(this);
    m_problemsInput->setReadOnly(true);
    m_problemsInput->setLineWrapMode(QPlainTextEdit::NoWrap);
    m_problemsInput->setMaximumHeight(100);
    m_problemsInput->setVisible(false);

    QWidget* btnsWidget = new QWidget(this);

    m_openDirBtn = new QPushButton("Open backup folder", btnsWidget);
    m_openDirBtn->setAutoDefault(false);
    m_openDirBtn->setVisible(false);

    m_cancelBtn = new QPushButton("Not now", btnsWidget);
    m_cancelBtn->setAutoDefault(false);
    m_cancelBtn->setToolTip("Keep the old backups as they are and ask again on next start");

    m_confirmBtn = new QPushButton("Migrate", btnsWidget);
    m_confirmBtn->setObjectName("save_btn");
    m_confirmBtn->setDefault(true);

    QHBoxLayout* btnsLayout = new QHBoxLayout(btnsWidget);
    btnsLayout->setContentsMargins(0, 0, 0, 0);
    btnsLayout->setSpacing(10);
    btnsLayout->addWidget(m_openDirBtn, 0);
    btnsLayout->addStretch(1);
    btnsLayout->addWidget(m_cancelBtn, 0);
    btnsLayout->addWidget(m_confirmBtn, 0);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(15, 15, 15, 15);
    layout->setSpacing(10);
    layout->addWidget(m_titleLabel, 0);
    layout->addWidget(m_messageLabel, 0);
    layout->addWidget(m_gamesLabel, 0);
    layout->addWidget(m_progressBar, 0);
    layout->addWidget(m_problemsInput, 0);
    layout->addWidget(btnsWidget, 0);

    uintmax_t totalSize = 0;
    std::map<std::string, int> countByGame;
    for (const auto& entry : m_entries) {
        totalSize += entry.size;
        countByGame[entry.meta.game.toString()]++;
    }

    QString message = QString(
        "Backups are now kept in one database file instead of separate folders.<br><br>"
        "Found <b>%1 backup(s)</b> (%2) in the old format. They are copied first, the old folders "
        "are removed only after everything was saved."
    ).arg(m_entries.size()).arg(locale().formattedDataSize(static_cast<qint64>(totalSize)));
    m_messageLabel->setText(message);

    QStringList gameLines;
    for (const auto& [game, count] : countByGame)
        gameLines.append(QString("%1: %2").arg(QString::fromStdString(game)).arg(count));
    m_gamesLabel->setText(gameLines.join("\n"));

    if (!m_skipped.isEmpty()) {
        m_gamesLabel->setText(
            m_gamesLabel->text()
            + QString("\n\n%1 folder(s) can't be migrated and will be left untouched:").arg(m_skipped.size())
        );
        showProblems(m_skipped);
    }

    connect(m_confirmBtn, &QPushButton::clicked, this, [this]() {
        if (m_state == State::Confirm) {
            startMigration();
        } else {
            accept();
        }
    });
    connect(m_cancelBtn, &QPushButton::clicked, this, &MigrationDialog::reject);
    connect(m_openDirBtn, &QPushButton::clicked, this, [this]() {
        QDesktopServices::openUrl(QUrl::fromLocalFile(m_backupRoot));
    });
}

void MigrationDialog::reject() {
    switch (m_state) {
        case State::Copying:
            // Next step would not start, everything inserted so far is discarded
            m_db->rollback();
            m_state = State::Confirm;
            QDialog::reject();
            break;

        case State::Removing:
            // Backups are already in the database, finish the cleanup
            break;

        default:
            QDialog::reject();
            break;
    }
}

void MigrationDialog::startMigration() {
    if (!m_db->beginTransaction()) {
        showFailure("Could not access the backups database.");
        return;
    }
    m_state = State::Copying;
    m_titleLabel->setText("Migrating backups...");
    m_gamesLabel->setVisible(false);
    m_problemsInput->setVisible(false);
    m_progressBar->setVisible(true);
    m_cancelBtn->setText("Cancel");
    m_cancelBtn->setToolTip("Stop and keep the old backups as they are");
    m_confirmBtn->setVisible(false);
    adjustSize();
    QTimer::singleShot(0, this, [this]() { copyNextEntry(); });
}

void MigrationDialog::copyNextEntry() {
    if (m_state != State::Copying) return;

    if (m_nextIdx >= m_entries.size()) {
        if (!m_db->commit()) {
            m_db->rollback();
            showFailure("Could not save the backups to the database.");
            return;
        }
        m_state = State::Removing;
        m_nextIdx = 0;
        m_migrated = static_cast<int>(m_migratedDirs.size());
        m_titleLabel->setText("Removing old backup folders...");
        m_messageLabel->setText("All backups were saved to the database.");
        m_progressBar->setRange(0, static_cast<int>(m_migratedDirs.size()));
        m_progressBar->setValue(0);
        m_cancelBtn->setEnabled(false);
        QTimer::singleShot(0, this, [this]() { removeNextDir(); });
        return;
    }

    const LegacyEntry& entry = m_entries[m_nextIdx];
    m_messageLabel->setText(
        QString("%1: backup %2 of %3")
            .arg(entry.meta.game.toString())
            .arg(m_nextIdx + 1)
            .arg(m_entries.size())
    );

    std::ifstream ifs(entry.backupDir / entry.meta.filename, std::ios::binary);
    std::vector<char> buf(std::istreambuf_iterator<char>(ifs), {});
    if (buf.size() != entry.size) {
        // Keep the directory, the backup would be lost if it was removed with partial data
        std::cerr << "MigrationHelper: failed to read save file in " << entry.backupDir << "\n";
        m_skipped.append(backupDirLabel(entry.backupDir) + ": could not read the save file");
    } else {
        QByteArray saveData(buf.data(), static_cast<qsizetype>(buf.size()));
        if (!m_db->insertBackup(entry.meta, saveData)) {
            m_db->rollback();
            showFailure("Could not write a backup to the database. Is there enough free disk space?");
            return;
        }
        m_migratedDirs.push_back(entry.backupDir);
    }

    m_nextIdx++;
    m_progressBar->setValue(static_cast<int>(m_nextIdx));
    QTimer::singleShot(0, this, [this]() { copyNextEntry(); });
}

void MigrationDialog::removeNextDir() {
    if (m_nextIdx >= m_migratedDirs.size()) {
        // Also remove parent game dirs if now empty
        std::unordered_set<std::wstring> gameDirs;
        for (const auto& dir : m_migratedDirs)
            gameDirs.insert(dir.parent_path().wstring());
        for (const auto& gameDir : gameDirs) {
            std::error_code ec;
            if (std::filesystem::is_empty(gameDir, ec) && !ec)
                std::filesystem::remove(gameDir, ec);
        }
        m_state = State::Finished;
        showResult();
        return;
    }

    const auto& dir = m_migratedDirs[m_nextIdx];
    std::error_code ec;
    std::filesystem::remove_all(dir, ec);
    if (ec) {
        std::cerr << "MigrationHelper: failed to remove " << dir << ": " << ec.message() << "\n";
        m_notRemoved.append(backupDirLabel(dir));
    }

    m_nextIdx++;
    m_progressBar->setValue(static_cast<int>(m_nextIdx));
    QTimer::singleShot(0, this, [this]() { removeNextDir(); });
}

void MigrationDialog::showFailure(const QString& reason) {
    m_state = State::Finished;
    m_titleLabel->setText("Migration failed");
    m_messageLabel->setText(
        reason + "<br><br>No changes were made, your old backups are untouched. "
        "The migration will be offered again on next start."
    );
    m_gamesLabel->setVisible(false);
    m_progressBar->setVisible(false);
    m_problemsInput->setVisible(false);
    m_cancelBtn->setVisible(false);
    m_confirmBtn->setText("Close");
    m_confirmBtn->setVisible(true);
    adjustSize();
}

void MigrationDialog::showResult() {
    QStringList problems = m_skipped;
    for (const auto& dir : m_notRemoved)
        problems.append(dir + ": already migrated, safe to delete");

    m_titleLabel->setText(problems.isEmpty() ? "Migration finished" : "Migration finished with warnings");
    QString message = QString("%1 backup(s) migrated.").arg(m_migrated);
    if (!m_skipped.isEmpty())
        message.push_back(QString(" %1 not migrated and left in place.").arg(m_skipped.size()));
    if (!m_notRemoved.isEmpty())
        message.push_back(QString(" %1 old folder(s) could not be removed.").arg(m_notRemoved.size()));
    m_messageLabel->setText(message);

    m_progressBar->setVisible(false);
    m_cancelBtn->setVisible(false);
    m_confirmBtn->setText("Close");
    m_confirmBtn->setVisible(true);
    if (!problems.isEmpty()) {
        m_openDirBtn->setVisible(true);
        showProblems(problems);
    }
    adjustSize();
}

void MigrationDialog::showProblems(const QStringList& problems) {
    m_problemsInput->setPlainText(problems.join("\n"));
    m_problemsInput->setVisible(true);
}
}

bool migrateIfAgreed(const QString& backupRoot, BackupsDb* db, QWidget* parent) {
    LegacyScan scan = scanLegacyBackups(backupRoot);
    // Nothing to offer when only broken backups are left
    if (scan.entries.empty()) return false;

    MigrationDialog dialog(backupRoot, std::move(scan), db, parent);
    dialog.exec();
    return dialog.isMigrated();
}
