#pragma once

#include <QThread>
#include <unordered_set>

#include "KeysWindows.h"
#include "ConfigModel.h"
#include "BackupsModel.h"
#include "../parse/DSRSaveFile.h"

// Handler of hotkeys presss
// - check for pressed keys using Windows api
class HotkeysThread: public QThread {
    Q_OBJECT
signals:
    void quickSaveRequested();
    void quickLoadRequested();

public:
    explicit HotkeysThread(const ConfigHotkeys& hotkeys, QObject* parent);
    void stop();
    void updateHotkeys(const ConfigHotkeys& hotkeys);

protected:
    void run() override;
private:
    bool m_isRunning = false;
    bool m_hotkeysChanged = false;
    std::unordered_set<int> m_quickSaveHotkey = {};
    std::unordered_set<int> m_quickLoadHotkey = {};
};

// Thread periodically checking if save file changed
// NOTE Maybe 'QFileSystemWatcher' could be used instead? How to handle which save id it is related to?
class SaveChangesThread: public QThread {
    Q_OBJECT
signals:
    void saveFileChanged(QString);
public:
    explicit SaveChangesThread(const std::vector<SaveFileItem>& saveItems, QObject* parent);
    void updatePaths(const std::vector<SaveFileItem>& saveItems);
    void stop();

protected:
    void run() override;
private:
    bool m_isRunning = false;
    std::unordered_map<QString, std::filesystem::path> m_saveFilesBySaveId;
    std::unordered_map<QString, std::filesystem::file_time_type> m_lastChangedById;
};

// Result to receive characters of DSR save file
struct DSRCharInfoResult {
    QString error;
    std::vector<fsm::parse::DSRCharacterInfo> characters;
};

// Controller wrapping backend logic allowing UI to access data it needs
class Controller: public QObject {
    Q_OBJECT
signals:
    void gameSaveChanged(fsm::parse::Game game);
    void saveIdChanged(QString saveId);
    // TODO find out if are used?
    void pathsConfigChanged();
    void hotkeysConfigChanged();
    void autobackupConfigChanged();

public:
    explicit Controller(QObject* parent = nullptr);
    ~Controller() override;

    QString getLastSelectedSaveId() const;
    void setCurrentTabId(const QString& saveId);
    ConfigSettingsData getConfigSettingsData() const;
    ConfigHotkeys getHotkeysConfig() const;
    void saveConfigData(const ConfigConfirmData& confirmData);

    std::vector<SaveFileItem> getSaveFileItems() const;
    DSRCharInfoResult getDsrCharacters(const QString& saveId) const;
    // DS2CharInfoResult getDs2Characters(const QString& saveId) const;
    // DS3CharInfoResult getDs3Characters(const QString& saveId) const;
    // SekiroCharInfoResult getSekiroCharacters(const QString& saveId) const;
    // ERCharInfoResult getERCharacters(const QString& saveId) const;

    void createManualBackup(const QString& label);
    void deleteBackupByIds(const std::vector<QString>& backupIds);

    void openBackupDir();

private slots:
    void onQuickSaveRequest();
    void onQuickLoadRequest();
    void onGamePathsChange();
    void onHotkeysChange();
    void onAutobackupChange();
    void onSaveFileChange(const QString& saveId);

private:
    QString m_currentSaveId = "";
    ConfigModel* m_configModel;
    BackupsModel* m_backupsModel;
    HotkeysThread* m_hotkeysThread;
    SaveChangesThread* m_saveChangesThread;
};
