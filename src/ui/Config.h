#pragma once
#include <QObject>

#include "../parse/DSRSaveFile.h"

// How UI receives information about available save files.
// Each item contains game and save id. The combination can be used to receive available information about save.
struct SaveFileItem {
    fsm::parse::Game game;
    QString saveId;
};

// Information about default location of game save files
struct DefaultSavePathInfo {
    QString savePath {};
    QString savePathHint {};
    bool saveFileExists = false;
};

// --------------------------
// structs to keep track about current config
// --------------------------
struct ConfigSavePathData {
    QString savePath {};
    QString saveId {};
    bool isSet = false;
};

struct ConfigGameSavePaths {
    ConfigSavePathData dsrSavePath {};
    ConfigSavePathData ds2SavePath {};
    ConfigSavePathData ds3SavePath {};
    ConfigSavePathData sekiroSavePath {};
    ConfigSavePathData erSavePath {};
};

struct ConfigHotkeys {
    QKeyCombination quickSaveHotkey = QKeyCombination();
    QKeyCombination quickLoadHotkey = QKeyCombination();
};

struct ConfigAutobackup {
    bool enabled = false;
    int frequency = 60;
    int maxBackups = 10;
};

struct ConfigData {
    bool isLoaded = false;
    ConfigGameSavePaths gameSaveFiles {};
    ConfigHotkeys hotkeys {};
    ConfigAutobackup autobackup {};
};

// --------------------------
// structs related to passing config information to UI and receive changes from UI
// --------------------------
struct ConfigSavePathInfo {
    QString savePath;
    QString savePathHint;
    QString savePathDefault;
};

struct ConfigInfo {
    ConfigSavePathInfo dsrSavePath;
    ConfigSavePathInfo ds2SavePath;
    ConfigSavePathInfo ds3SavePath;
    ConfigSavePathInfo sekiroSavePath;
    ConfigSavePathInfo erSavePath;

    QKeyCombination quickSaveHotkey;
    QKeyCombination quickLoadHotkey;

    bool autobackupEnabled;
    int autobackupFrequency;
    int maxAutobackups;
};

class ConfigConfirmInfo {

};

class Config: public QObject {
signals:
    void pathsChanged();
    void hotkeysChanged();
    void autoBackupChanged();
    void configChanged();
public:
    explicit Config(QObject* parent = nullptr);
    ConfigInfo getConfigInfo();
    void saveConfigInfo(const ConfigConfirmInfo& configInfo);
    void saveConfig();
    QString getBackupDirPath();
    std::vector<SaveFileItem> getSaveFileItems();
    std::optional<SaveFileItem> getSaveFileItem(const QString& saveId);
    std::optional<QString> getSavePathItem(const QString& saveId);
    std::optional<QString> getSaveIdByGame(const fsm::parse::Game& game);
    QString getLastSelectedSaveId();
    void setLastSelectedSaveId(const QString& saveId);
    DefaultSavePathInfo getDefaultSavePath(const fsm::parse::Game& game);
private:
    ConfigData m_configData;
    QString m_lastTabId = "";
    QString m_appConfigPath = "";
    QString m_appBackupsDir = "";
    std::unordered_map<QString, SaveFileItem> m_saveInfoById {};
    DefaultSavePathInfo m_defaultSavePath {};
    void p_loadConfig();
    void p_saveConfig();
    static DefaultSavePathInfo p_getDefaultDSRSavePath();
    static DefaultSavePathInfo p_getDefaultDS2SavePath();
    static DefaultSavePathInfo p_getDefaultDS3SavePath();
    static DefaultSavePathInfo p_getDefaultERSavePath();
    static DefaultSavePathInfo p_getDefaultSekiroSavePath();
};
