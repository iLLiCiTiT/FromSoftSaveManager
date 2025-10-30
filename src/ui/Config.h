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
    bool discovered = false;
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
    const DefaultSavePathInfo& getDefaultSavePath(const fsm::parse::Game& game);
private:
    DefaultSavePathInfo m_defaultSavePath;
    DefaultSavePathInfo m_defaultDSRSavePath;
    DefaultSavePathInfo m_defaultDS2SavePath;
    DefaultSavePathInfo m_defaultDS3SavePath;
    DefaultSavePathInfo m_defaultERSavePath;
    DefaultSavePathInfo m_defaultSekiroSavePath;
    void p_loadConfig();
    void p_saveConfig();
    const DefaultSavePathInfo& p_getDefaultSavePath(const fsm::parse::Game& game);
    const DefaultSavePathInfo& p_getDefaultDSRSavePath();
    const DefaultSavePathInfo& p_getDefaultDS2SavePath();
    const DefaultSavePathInfo& p_getDefaultDS3SavePath();
    const DefaultSavePathInfo& p_getDefaultERSavePath();
    const DefaultSavePathInfo& p_getDefaultSekiroSavePath();
};
