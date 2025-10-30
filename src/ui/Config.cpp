#include "Config.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <windows.h>
#include <shlobj.h>
#include <QUuid>
#include <nlohmann/json.hpp>

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

ConfigInfo Config::getConfigInfo() {
    return ConfigInfo {};
}

void Config::saveConfigInfo(const ConfigConfirmInfo &configInfo) {
}

void Config::saveConfig() {
}

QString Config::getBackupDirPath() {
    QString backupDir;
    return backupDir;
}

std::vector<SaveFileItem> Config::getSaveFileItems() {
    std::vector<SaveFileItem> output;
    // TODO implement
    return output;
}

std::optional<SaveFileItem> Config::getSaveFileItem(const QString &saveId) {
    return std::nullopt;
}

std::optional<QString> Config::getSavePathItem(const QString &saveId) {
    return std::nullopt;
}

std::optional<QString> Config::getSaveIdByGame(const fsm::parse::Game &game) {
    return std::nullopt;
}

QString Config::getLastSelectedSaveId() {
    QString saveId;
    return saveId;
}

void Config::setLastSelectedSaveId(const QString &saveId) {
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
    auto& autobackup = m_configData.autobackup;
    auto& hotkeys = m_configData.hotkeys;

}

void Config::p_saveConfig() {
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
