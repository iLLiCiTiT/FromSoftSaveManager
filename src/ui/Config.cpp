#include "Config.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <qevent.h>
#include <windows.h>
#include <shlobj.h>
#include <QUuid>
#include <nlohmann/json.hpp>

#include "KeysWindows.h"

using json = nlohmann::json;


static std::wstring GetWindowsDocumentsDirW()
{
    // SHGetKnownFolderPath typically expects COM initialized. We'll try to init here,
    // but continue even if it was already initialized in another mode.
    bool coInit = false;
    HRESULT hrInit = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (SUCCEEDED(hrInit)) {
        coInit = true;
    } else if (hrInit == RPC_E_CHANGED_MODE) {
        // COM already initialized with a different model; proceed without changing it.
    }

    PWSTR path = nullptr;
    HRESULT hr = SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &path);

    std::wstring result;
    if (SUCCEEDED(hr) && path) {
        result.assign(path);
        CoTaskMemFree(path);
    } else {
        // Fallback to %USERPROFILE%\Documents
        const wchar_t* userProfile = _wgetenv(L"USERPROFILE");
        if (userProfile && *userProfile) {
            result = std::wstring(userProfile) + L"\\Documents";
        } else {
            // give empty string as a last resort
            result.clear();
        }
    }

    if (coInit) {
        CoUninitialize();
    }

    return result;
}

static QString getAppDataPath(const QString& subfolder) {
    QString saveDir = QString::fromStdString(getenv("APPDATA"));
    saveDir.append("\\" + subfolder);
    return saveDir;
}

static QString getAppConfigDir() {
    QString saveDir = QString::fromStdString(getenv("LOCALAPPDATA"));
    saveDir.append("\\FromSoftSaveManager\\FromSoftSaveManager");
    return saveDir;
}

static std::string generateUniqueId() {
    QUuid uuid = QUuid::createUuid();
    QString s = uuid.toString(QUuid::WithoutBraces);
    return s.toStdString();
}

static QString findSaveFile(const std::string& dirpath, const std::string& filename) {
    if (std::filesystem::exists(dirpath)) {
        for (auto const& dirEntry : std::filesystem::directory_iterator{dirpath}) {
            if (!dirEntry.is_directory()) continue;
            std::filesystem::path path = std::filesystem::path(dirEntry.path());
            path += "\\" + filename;
            if (is_directory(path)) continue;
            if (!std::filesystem::exists(path)) continue;
            return QString::fromStdString(path.string());
        }
    }
    return QString {};
}

Config::Config(QObject* parent): QObject(parent) {
    m_appConfigPath = getAppConfigDir();
    m_appConfigPath.push_back("\\config.json");
    m_appBackupsDir = getAppConfigDir();
    m_appBackupsDir.push_back("\\backups");

    p_loadConfig();
}

ConfigSettingsData Config::getConfigSettingsData() {
    auto d_dsr = p_getDefaultDSRSavePath();
    auto d_ds2 = p_getDefaultDS2SavePath();
    auto d_ds3 = p_getDefaultDS3SavePath();
    auto d_er = p_getDefaultERSavePath();
    auto d_sekiro = p_getDefaultSekiroSavePath();

    return ConfigSettingsData {
        .dsrSavePath = {
            .savePath = m_configData.gameSaveFiles.dsrSavePath.savePath,
            .savePathHint = d_dsr.savePathHint,
            .savePathDefault = d_dsr.savePath,
        },
        .ds2SavePath = {
            .savePath = m_configData.gameSaveFiles.ds2SavePath.savePath,
            .savePathHint = d_ds2.savePathHint,
            .savePathDefault = d_ds2.savePath,
        },
        .ds3SavePath = {
            .savePath = m_configData.gameSaveFiles.ds3SavePath.savePath,
            .savePathHint = d_ds3.savePathHint,
            .savePathDefault = d_ds3.savePath,
        },
        .sekiroSavePath = {
            .savePath = m_configData.gameSaveFiles.sekiroSavePath.savePath,
            .savePathHint = d_sekiro.savePathHint,
            .savePathDefault = d_sekiro.savePath,
        },
        .erSavePath = {
            .savePath = m_configData.gameSaveFiles.erSavePath.savePath,
            .savePathHint = d_er.savePathHint,
            .savePathDefault = d_er.savePath,
        },

        .quickSaveHotkey = m_configData.hotkeys.quickSaveHotkey,
        .quickLoadHotkey = m_configData.hotkeys.quickLoadHotkey,

        .autobackupEnabled = m_configData.autobackup.enabled,
        .autobackupFrequency = m_configData.autobackup.frequency,
        .maxAutobackups = m_configData.autobackup.maxBackups,
    };
}

void Config::saveConfigData(const ConfigConfirmData& configData) {
    // TODO implement
}

void Config::saveConfig() {
    // TODO implement
}

QString Config::getBackupDirPath() {
    return m_appBackupsDir;
}

std::vector<SaveFileItem> Config::getSaveFileItems() {
    std::vector<SaveFileItem> output;

    const ConfigSavePathData& dsrSavePathInfo = m_configData.gameSaveFiles.dsrSavePath;
    const ConfigSavePathData& ds2SavePathInfo = m_configData.gameSaveFiles.ds2SavePath;
    const ConfigSavePathData& ds3SavePathInfo = m_configData.gameSaveFiles.ds3SavePath;
    const ConfigSavePathData& sekiroSavePathInfo = m_configData.gameSaveFiles.sekiroSavePath;
    const ConfigSavePathData& erSavePathInfo = m_configData.gameSaveFiles.erSavePath;
    if (dsrSavePathInfo.isSet)
        output.push_back({
            .game = fsm::parse::Game::DSR,
            .saveId = dsrSavePathInfo.saveId,
            .savePath = dsrSavePathInfo.savePath,
        });
    if (ds2SavePathInfo.isSet)
        output.push_back({
            .game = fsm::parse::Game::DS2_SOTFS,
            .saveId = ds2SavePathInfo.saveId,
            .savePath = ds2SavePathInfo.savePath,
        });
    if (ds3SavePathInfo.isSet)
        output.push_back({
            .game = fsm::parse::Game::DS3,
            .saveId = ds3SavePathInfo.saveId,
            .savePath = ds3SavePathInfo.savePath,
        });
    if (sekiroSavePathInfo.isSet)
        output.push_back({
            .game = fsm::parse::Game::Sekiro,
            .saveId = sekiroSavePathInfo.saveId,
            .savePath = sekiroSavePathInfo.savePath,
        });
    if (erSavePathInfo.isSet)
        output.push_back({
            .game = fsm::parse::Game::ER,
            .saveId = erSavePathInfo.saveId,
            .savePath = erSavePathInfo.savePath,
        });

    return output;
}

std::optional<SaveFileItem> Config::getSaveFileItem(const QString& saveId) {
    auto it = m_saveInfoById.find(saveId);
    if (it == m_saveInfoById.end()) return std::nullopt;
    return it->second;
}

std::optional<QString> Config::getSavePathItem(const QString& saveId) {
    // TODO find out if it needed?
    auto item = getSaveFileItem(saveId);
    if (item.has_value()) return item.value().savePath;
    return std::nullopt;
}

std::optional<QString> Config::getSaveIdByGame(const fsm::parse::Game& game) {
    switch (game) {
        case fsm::parse::Game::DSR:
            if (!m_configData.gameSaveFiles.dsrSavePath.isSet) return std::nullopt;
            return m_configData.gameSaveFiles.dsrSavePath.saveId;
        case fsm::parse::Game::DS2_SOTFS:
            if (!m_configData.gameSaveFiles.ds2SavePath.isSet) return std::nullopt;
            return m_configData.gameSaveFiles.ds2SavePath.saveId;
        case fsm::parse::Game::DS3:
            if (!m_configData.gameSaveFiles.ds3SavePath.isSet) return std::nullopt;
            return m_configData.gameSaveFiles.ds3SavePath.saveId;
        case fsm::parse::Game::Sekiro:
            if (!m_configData.gameSaveFiles.sekiroSavePath.isSet) return std::nullopt;
            return m_configData.gameSaveFiles.sekiroSavePath.saveId;
        case fsm::parse::Game::ER:
            if (!m_configData.gameSaveFiles.erSavePath.isSet) return std::nullopt;
            return m_configData.gameSaveFiles.erSavePath.saveId;
        default:
            return std::nullopt;
    }
}

QString Config::getLastSelectedSaveId() const {
    return m_configData.lastSaveId;
}

void Config::setLastSelectedSaveId(const QString &saveId) {
    if (!saveId.isEmpty())
        m_configData.lastSaveId = saveId;
}

void Config::p_loadConfig() {
    if (m_configData.isLoaded) return;
    m_configData.isLoaded = true;
    json data;
    if (!std::filesystem::exists(m_appConfigPath.toStdString())) {
        std::ifstream f(m_appConfigPath.toStdString());
        data = json::parse(f);
        f.close();
    }
    if (data.is_null()) {
        data = json::object();
    }
    if (!data.contains("game_save_files")) {
        data["game_save_files"] = json::object();
    }
    for (auto& game: std::initializer_list<fsm::parse::Game>{
        fsm::parse::Game::DSR,
        fsm::parse::Game::DS2_SOTFS,
        fsm::parse::Game::DS3,
        fsm::parse::Game::ER,
        fsm::parse::Game::Sekiro,
    }) {
        std::string gameName = game.toString();
        if (data["game_save_files"].contains(gameName)) continue;
        auto defaultPathInfo = getDefaultSavePath(game);
        if (!defaultPathInfo.saveFileExists) continue;
        json gameInfo = json::object();
        gameInfo["path"] = defaultPathInfo.savePath.toStdString();
        gameInfo["save_id"] = generateUniqueId();
        data["game_save_files"][gameName] = gameInfo;
    }
    auto& gameSaveFiles = m_configData.gameSaveFiles;
    for (auto& el: data["game_save_files"].items()) {
        json& info = el.value();
        if (!info.contains("save_id") || !info.contains("path")) continue;
        QString path = QString::fromStdString(info["path"]);
        QString saveId = QString::fromStdString(info["save_id"]);

        std::string_view gameName = el.key();
        fsm::parse::Game game = fsm::parse::Game::fromString(gameName);
        ConfigSavePathData savePathInfo = {
            .savePath = path,
            .saveId = saveId,
            .isSet = true,
        };
        if (game != fsm::parse::Game::Unknown)
            m_saveInfoById[saveId] = {
                .game = game,
                .saveId = saveId,
                .savePath = path
            };
        switch (game) {
            case fsm::parse::Game::DSR:
                gameSaveFiles.dsrSavePath = savePathInfo;
                break;
            case fsm::parse::Game::DS2_SOTFS:
                gameSaveFiles.ds2SavePath = savePathInfo;
                break;
            case fsm::parse::Game::DS3:
                gameSaveFiles.ds3SavePath = savePathInfo;
                break;
            case fsm::parse::Game::Sekiro:
                gameSaveFiles.sekiroSavePath = savePathInfo;
                break;
            case fsm::parse::Game::ER:
                gameSaveFiles.erSavePath = savePathInfo;
                break;
            default:
                break;
        }
    }

    // Hotkeys
    if (!data.contains("hotkeys")) {
        data["hotkeys"] = json::object();
    }
    if (!data["hotkeys"].contains("quicksave")) {
        data["hotkeys"]["quicksave"] = qtCombinationToInt(QKeyCombination(Qt::Key_F5));
    }

    if (!data["hotkeys"].contains("quickload")) {
        data["hotkeys"]["quickload"] = qtCombinationToInt(QKeyCombination(Qt::Key_F8));
    }
    auto& hotkeys = m_configData.hotkeys;
    std::unordered_set<int> quicksave = data["hotkeys"]["quicksave"];
    std::unordered_set<int> quickload = data["hotkeys"]["quickload"];
    hotkeys.quickSaveHotkey = intCombinationToQt(quicksave);
    hotkeys.quickLoadHotkey = intCombinationToQt(quickload);

    // Autobackup
    auto& autobackup = m_configData.autobackup;
    if (!data.contains("autobackup")) {
        data["autobackup"] = json::object();
    }
    json& autobackupData = data["autobackup"];

    if (autobackupData.contains("enabled") && autobackupData["enabled"].is_boolean()) {
        autobackup.enabled = autobackupData["enabled"];
    }
    if (autobackupData.contains("frequency") && autobackupData["frequency"].is_number()) {
        autobackup.frequency = autobackupData["frequency"];
    }
    if (autobackupData.contains("max_autobackups") && autobackupData["max_autobackups"].is_number()) {
        autobackup.maxBackups = autobackupData["max_autobackups"];
    }

    // Last selected save id
    if (data.contains("last_selected_save_id") && data["last_selected_save_id"].is_string()) {
        m_configData.lastSaveId = QString::fromStdString(data["last_selected_save_id"]);
    }
}

void Config::p_saveConfig() {
    // TODO implement
}

DefaultSavePathInfo Config::getDefaultSavePath(const fsm::parse::Game& game) {
    switch (game) {
        case fsm::parse::Game::DSR: return p_getDefaultDSRSavePath();
        case fsm::parse::Game::DS2_SOTFS: return p_getDefaultDS2SavePath();
        case fsm::parse::Game::DS3: return p_getDefaultDS3SavePath();
        case fsm::parse::Game::ER: return p_getDefaultERSavePath();
        case fsm::parse::Game::Sekiro: return p_getDefaultSekiroSavePath();
        default: return m_defaultSavePath;
    }
}

DefaultSavePathInfo Config::p_getDefaultDSRSavePath() {
    DefaultSavePathInfo output;

    output.savePathHint = QString::fromStdWString(GetWindowsDocumentsDirW());
    output.savePath = QString {};
    output.saveFileExists = false;
    if (!output.savePathHint.isEmpty()) {
        output.savePathHint.push_back("\\NBGI\\DARK SOULS REMASTERED");
        if (std::filesystem::exists(output.savePathHint.toStdString())) {
            for (auto const& dirEntry : std::filesystem::directory_iterator{output.savePathHint.toStdString()}) {
                if (!dirEntry.is_directory()) continue;
                std::filesystem::path path = std::filesystem::path(dirEntry.path());
                path += "\\DRAKS0005.sl2";
                if (is_directory(path)) continue;
                if (!std::filesystem::exists(path)) continue;
                output.saveFileExists = true;
                output.savePath = QString::fromStdString(path.string());
                break;
            }
        }
    }
    return output;
}

DefaultSavePathInfo Config::p_getDefaultDS2SavePath() {
    DefaultSavePathInfo output;
    output.savePathHint = getAppDataPath("DarkSoulsII");
    output.savePath = findSaveFile(
        output.savePathHint.toStdString(),
        "DS2SOFS0000.sl2"
    );
    output.saveFileExists = !output.savePath.isEmpty();
    return output;
}

DefaultSavePathInfo Config::p_getDefaultDS3SavePath() {
    DefaultSavePathInfo output;
    output.savePathHint = getAppDataPath("DarkSoulsIII");
    output.savePath = findSaveFile(
        output.savePathHint.toStdString(),
        "DS30000.sl2"
    );
    output.saveFileExists = !output.savePath.isEmpty();
    return output;
}

DefaultSavePathInfo Config::p_getDefaultERSavePath() {
    DefaultSavePathInfo output;
    output.savePathHint = getAppDataPath("EldenRing");
    output.savePath = findSaveFile(
        output.savePathHint.toStdString(),
        "ER0000.sl2"
    );
    output.saveFileExists = !output.savePath.isEmpty();
    return output;
}

DefaultSavePathInfo Config::p_getDefaultSekiroSavePath() {
    DefaultSavePathInfo output;
    output.savePathHint = getAppDataPath("Sekiro");
    output.savePath = findSaveFile(
        output.savePathHint.toStdString(),
        "S0000.sl2"
    );
    output.saveFileExists = !output.savePath.isEmpty();
    return output;
}
