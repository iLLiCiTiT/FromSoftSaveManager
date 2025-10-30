#pragma once
#include <QObject>

#include "../parse/DSRSaveFile.h"

struct SaveFileItem {
    fsm::parse::Game game;
    QString saveId;
};

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

struct DefaultSavePathInfo {
    QString savePath;
    QString savePathHint;
    bool saveFileExists;
};

class Config: public QObject {
signals:
    void pathsChanged();
    void hotkeysChanged();
    void autoBackupChanged();
    void configChanged();
public:
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
    QString m_lastTabId = "";
    DefaultSavePathInfo m_defaultSavePath {};
    void p_loadConfig();
    void p_saveConfig();
    static DefaultSavePathInfo p_getDefaultDSRSavePath();
    static DefaultSavePathInfo p_getDefaultDS2SavePath();
    static DefaultSavePathInfo p_getDefaultDS3SavePath();
    static DefaultSavePathInfo p_getDefaultERSavePath();
    static DefaultSavePathInfo p_getDefaultSekiroSavePath();
};
