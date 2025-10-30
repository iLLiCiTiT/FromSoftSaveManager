#include "Config.h"

#include <filesystem>
#include <iostream>
#include <locale>
#include <windows.h>
#include <shlobj.h>


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
}

void Config::p_saveConfig() {
}

const DefaultSavePathInfo& Config::getDefaultSavePath(const fsm::parse::Game& game) {
    return p_getDefaultSavePath(game);
}

const DefaultSavePathInfo Config::p_getDefaultSavePath(const fsm::parse::Game& game) {
    switch (game) {
        case fsm::parse::Game::DSR:
            return p_getDefaultDSRSavePath();
        case fsm::parse::Game::DS2_SOTFS:
            return p_getDefaultDS2SavePath();
        case fsm::parse::Game::DS3:
            return p_getDefaultDS3SavePath();
        case fsm::parse::Game::ER:
            return p_getDefaultERSavePath();
        case fsm::parse::Game::Sekiro:
            return p_getDefaultSekiroSavePath();
    }
    return m_defaultSavePath;
}

const DefaultSavePathInfo Config::p_getDefaultDSRSavePath() {
    DefaultSavePathInfo output;

    QString saveDir = QString::fromStdWString(GetWindowsDocumentsDirW());
    QString savePath;
    output.saveFileExists = false;
    if (!saveDir.isEmpty()) {
        saveDir.push_back("\\NBGI\\DARK SOULS REMASTERED");
        if (std::filesystem::exists(saveDir.toStdString())) {
            for (auto const& dirEntry : std::filesystem::directory_iterator{saveDir.toStdString()}) {
                if (!dirEntry.is_directory()) continue;
                std::filesystem::path path = std::filesystem::path(dirEntry.path());
                path += "\\DRAKS0005.sl2";
                if (is_directory(path)) continue;
                if (!std::filesystem::exists(path)) continue;
                output.saveFileExists = true;
                savePath.push_back(QString::fromStdString(path.string()));
                break;
            }
        }
    }
    output.savePathHint = saveDir;
    output.savePath = savePath;

    return output;
}

const DefaultSavePathInfo Config::p_getDefaultDS2SavePath() {
    DefaultSavePathInfo output;
    output.savePathHint = getAppDataPath("DarkSoulsII");
    output.savePath = findSaveFile(
        output.savePathHint.toStdString(),
        "DS2SOFS0000.sl2"
    );
    if (!output.savePath.isEmpty()) output.saveFileExists = true;
    return output;
}

const DefaultSavePathInfo Config::p_getDefaultDS3SavePath() {
    DefaultSavePathInfo output;
    output.savePathHint = getAppDataPath("DarkSoulsIII");
    output.savePath = findSaveFile(
        output.savePathHint.toStdString(),
        "DS30000.sl2"
    );
    if (!output.savePath.isEmpty()) output.saveFileExists = true;
    return output;
}

const DefaultSavePathInfo Config::p_getDefaultERSavePath() {
    DefaultSavePathInfo output;
    output.savePathHint = getAppDataPath("EldenRing");
    output.savePath = findSaveFile(
        output.savePathHint.toStdString(),
        "ER0000.sl2"
    );
    if (!output.savePath.isEmpty()) output.saveFileExists = true;
    return output;
}

const DefaultSavePathInfo Config::p_getDefaultSekiroSavePath() {
    DefaultSavePathInfo output;
    output.savePathHint = getAppDataPath("Sekiro");
    output.savePath = findSaveFile(
        output.savePathHint.toStdString(),
        "S0000.sl2"
    );
    if (!output.savePath.isEmpty()) output.saveFileExists = true;
    return output;
}
