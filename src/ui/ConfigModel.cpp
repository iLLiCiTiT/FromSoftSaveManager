#include "ConfigModel.h"

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

ConfigModel::ConfigModel(QObject* parent): QObject(parent) {
    m_appConfigPath = getAppConfigDir();
    m_appConfigPath.push_back("\\config.json");
    m_appBackupsDir = getAppConfigDir();
    m_appBackupsDir.push_back("\\backups");

    p_loadConfig();
}

ConfigSettingsData ConfigModel::getConfigSettingsData() {
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

void ConfigModel::saveConfigData(const ConfigConfirmData& confirmData) {
    bool l_pathsChanged = false;
    bool l_hotkeyChanged = false;
    bool l_autobackupChanged = false;

    if (confirmData.dsrSavePath.has_value()) {
        l_pathsChanged = true;
        auto& dsrSavePath = m_configData.gameSaveFiles.dsrSavePath;
        dsrSavePath.savePath = confirmData.dsrSavePath.value();
        dsrSavePath.isSet = std::filesystem::exists(dsrSavePath.savePath.toStdString());
        if (dsrSavePath.saveId.isEmpty())
            dsrSavePath.saveId = QString::fromStdString(generateUniqueId());
    }
    if (confirmData.ds2SavePath.has_value()) {
        l_pathsChanged = true;
        auto& ds2SavePath = m_configData.gameSaveFiles.ds2SavePath;
        ds2SavePath.savePath = confirmData.ds2SavePath.value();
        ds2SavePath.isSet = std::filesystem::exists(ds2SavePath.savePath.toStdString());
        if (ds2SavePath.saveId.isEmpty())
            ds2SavePath.saveId = QString::fromStdString(generateUniqueId());
    }
    if (confirmData.ds3SavePath.has_value()) {
        l_pathsChanged = true;
        auto& ds3SavePath = m_configData.gameSaveFiles.ds3SavePath;
        ds3SavePath.savePath = confirmData.ds2SavePath.value();
        ds3SavePath.isSet = std::filesystem::exists(ds3SavePath.savePath.toStdString());
        if (ds3SavePath.saveId.isEmpty())
            ds3SavePath.saveId = QString::fromStdString(generateUniqueId());
    }
    if (confirmData.sekiroSavePath.has_value()) {
        l_pathsChanged = true;
        auto& sekiroSavePath = m_configData.gameSaveFiles.sekiroSavePath;
        sekiroSavePath.savePath = confirmData.ds2SavePath.value();
        sekiroSavePath.isSet = std::filesystem::exists(sekiroSavePath.savePath.toStdString());
        if (sekiroSavePath.saveId.isEmpty())
            sekiroSavePath.saveId = QString::fromStdString(generateUniqueId());
    }
    if (confirmData.erSavePath.has_value()) {
        l_pathsChanged = true;
        auto& erSavePath = m_configData.gameSaveFiles.erSavePath;
        erSavePath.savePath = confirmData.ds2SavePath.value();
        erSavePath.isSet = std::filesystem::exists(erSavePath.savePath.toStdString());
        if (erSavePath.saveId.isEmpty())
            erSavePath.saveId = QString::fromStdString(generateUniqueId());
    }

    if (l_pathsChanged) p_updateInfoById();

    if (confirmData.quickSaveHotkey.has_value()) {
        l_hotkeyChanged = true;
        m_configData.hotkeys.quickSaveHotkey = confirmData.quickSaveHotkey.value();
    }
    if (confirmData.quickLoadHotkey.has_value()) {
        l_hotkeyChanged = true;
        m_configData.hotkeys.quickLoadHotkey = confirmData.quickLoadHotkey.value();
    }

    if (confirmData.autobackupEnabled.has_value()) {
        l_autobackupChanged = true;
        m_configData.autobackup.enabled = confirmData.autobackupEnabled.value();
    }
    if (confirmData.autobackupFrequency.has_value()) {
        l_autobackupChanged = true;
        m_configData.autobackup.frequency = confirmData.autobackupFrequency.value();
    }
    if (confirmData.maxAutobackups.has_value()) {
        l_autobackupChanged = true;
        m_configData.autobackup.maxBackups = confirmData.maxAutobackups.value();
    }
    bool changed = l_pathsChanged || l_hotkeyChanged || l_autobackupChanged;
    if (!changed) return;

    saveConfig();

    if (l_pathsChanged) emit pathsChanged();
    if (l_hotkeyChanged) emit hotkeysChanged();
    if (l_autobackupChanged) emit autoBackupChanged();
    emit configChanged();
}

void ConfigModel::saveConfig() {
    std::string dirPath = getAppConfigDir().toStdString();
    if (!std::filesystem::exists(dirPath)) {
        std::filesystem::create_directory(dirPath);
    }
    json jsonData = p_configToJson();
    std::ofstream o(m_appConfigPath.toStdString());
    o << jsonData.dump(4) << std::endl;
    o.close();
}

QString ConfigModel::getBackupDirPath() {
    return m_appBackupsDir;
}

std::vector<SaveFileItem> ConfigModel::getSaveFileItems() {
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

std::optional<QString> ConfigModel::getSavePathItem(const QString& saveId) {
    auto it = m_saveInfoById.find(saveId);
    if (it == m_saveInfoById.end()) return std::nullopt;
    return it->second.savePath;
}

std::optional<QString> ConfigModel::getSaveIdByGame(const fsm::parse::Game& game) {
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

QString ConfigModel::getLastSelectedSaveId() const {
    return m_configData.lastSaveId;
}

void ConfigModel::setLastSelectedSaveId(const QString &saveId) {
    if (!saveId.isEmpty())
        m_configData.lastSaveId = saveId;
}

ConfigHotkeys ConfigModel::getHotkeysConfig() {
    return m_configData.hotkeys;
}

void ConfigModel::p_loadConfig() {
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
    p_updateInfoById();

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

json ConfigModel::p_configToJson() {
    json game_save_files = json::object();
    for (auto& item: getSaveFileItems()) {
        if (item.savePath.isEmpty()) continue;
        if (!std::filesystem::exists(item.savePath.toStdString())) continue;
        game_save_files[item.game.toString()] = {
            {"path", item.savePath.toStdString()},
            {"save_id", item.saveId.toStdString()},
        };
    }

    json hotkeys = json::object();
    hotkeys["quicksave"] = qtCombinationToInt(m_configData.hotkeys.quickSaveHotkey);
    hotkeys["quickload"] = qtCombinationToInt(m_configData.hotkeys.quickLoadHotkey);

    json autobackup = json::object();
    autobackup["enabled"] = m_configData.autobackup.enabled;
    autobackup["frequency"] = m_configData.autobackup.frequency;
    autobackup["max_autobackups"] = m_configData.autobackup.maxBackups;

    json data = json::object();
    data["game_save_files"] = game_save_files;
    data["hotkeys"] = hotkeys;
    data["autobackup"] = autobackup;
    data["last_selected_save_id"] = m_configData.lastSaveId.toStdString();
    return data;
}

DefaultSavePathInfo ConfigModel::getDefaultSavePath(const fsm::parse::Game& game) {
    switch (game) {
        case fsm::parse::Game::DSR: return p_getDefaultDSRSavePath();
        case fsm::parse::Game::DS2_SOTFS: return p_getDefaultDS2SavePath();
        case fsm::parse::Game::DS3: return p_getDefaultDS3SavePath();
        case fsm::parse::Game::ER: return p_getDefaultERSavePath();
        case fsm::parse::Game::Sekiro: return p_getDefaultSekiroSavePath();
        default: return m_defaultSavePath;
    }
}

void ConfigModel::p_updateInfoById() {
    m_saveInfoById.clear();
    auto& dsrSavePath = m_configData.gameSaveFiles.dsrSavePath;
    auto& ds2SavePath = m_configData.gameSaveFiles.ds2SavePath;
    auto& ds3SavePath = m_configData.gameSaveFiles.ds3SavePath;
    auto& erSavePath = m_configData.gameSaveFiles.erSavePath;
    auto& sekiroSavePath = m_configData.gameSaveFiles.sekiroSavePath;
    if (dsrSavePath.isSet) {
        m_saveInfoById[dsrSavePath.saveId] = {
            .game = fsm::parse::Game::DSR,
            .saveId = dsrSavePath.saveId,
            .savePath = dsrSavePath.savePath
        };
    }
    if (ds2SavePath.isSet) {
        m_saveInfoById[ds2SavePath.saveId] = {
            .game = fsm::parse::Game::DSR,
            .saveId = ds2SavePath.saveId,
            .savePath = ds2SavePath.savePath
        };
    }
    if (ds3SavePath.isSet) {
        m_saveInfoById[ds3SavePath.saveId] = {
            .game = fsm::parse::Game::DS2_SOTFS,
            .saveId = ds3SavePath.saveId,
            .savePath = ds3SavePath.savePath
        };
    }
    if (erSavePath.isSet) {
        m_saveInfoById[erSavePath.saveId] = {
            .game = fsm::parse::Game::DS3,
            .saveId = erSavePath.saveId,
            .savePath = erSavePath.savePath
        };
    }
    if (sekiroSavePath.isSet) {
        m_saveInfoById[sekiroSavePath.saveId] = {
            .game = fsm::parse::Game::Sekiro,
            .saveId = sekiroSavePath.saveId,
            .savePath = sekiroSavePath.savePath
        };
    }
}

DefaultSavePathInfo ConfigModel::p_getDefaultDSRSavePath() {
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

DefaultSavePathInfo ConfigModel::p_getDefaultDS2SavePath() {
    DefaultSavePathInfo output;
    output.savePathHint = getAppDataPath("DarkSoulsII");
    output.savePath = findSaveFile(
        output.savePathHint.toStdString(),
        "DS2SOFS0000.sl2"
    );
    output.saveFileExists = !output.savePath.isEmpty();
    return output;
}

DefaultSavePathInfo ConfigModel::p_getDefaultDS3SavePath() {
    DefaultSavePathInfo output;
    output.savePathHint = getAppDataPath("DarkSoulsIII");
    output.savePath = findSaveFile(
        output.savePathHint.toStdString(),
        "DS30000.sl2"
    );
    output.saveFileExists = !output.savePath.isEmpty();
    return output;
}

DefaultSavePathInfo ConfigModel::p_getDefaultERSavePath() {
    DefaultSavePathInfo output;
    output.savePathHint = getAppDataPath("EldenRing");
    output.savePath = findSaveFile(
        output.savePathHint.toStdString(),
        "ER0000.sl2"
    );
    output.saveFileExists = !output.savePath.isEmpty();
    return output;
}

DefaultSavePathInfo ConfigModel::p_getDefaultSekiroSavePath() {
    DefaultSavePathInfo output;
    output.savePathHint = getAppDataPath("Sekiro");
    output.savePath = findSaveFile(
        output.savePathHint.toStdString(),
        "S0000.sl2"
    );
    output.saveFileExists = !output.savePath.isEmpty();
    return output;
}
