#pragma once

#include <QThread>
#include <unordered_set>

#include "KeysWindows.h"
#include "ConfigModel.h"
#include "BackupsModel.h"
#include "../parse/DSRSaveFile.h"


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

struct DSRCharInfoResult {
    QString error;
    std::vector<fsm::parse::DSRCharacterInfo> characters;
};

class Controller: public QObject {
    Q_OBJECT
signals:
    void pathsChanged();
    void gameSaveChanged(fsm::parse::Game game);
    void saveIdChanged(QString saveId);
    void hotkeysChanged();
    void autobackupConfigChanged();

public:
    explicit Controller(QObject* parent = nullptr);
    ~Controller() override;

    QString getLastSelectedSaveId() const;
    void setCurrentTabId(const QString& saveId);
    ConfigSettingsData getConfigSettingsData() const;
    void saveConfigData(const ConfigConfirmData& confirmData);

    std::vector<SaveFileItem> getSaveFileItems() const;
    DSRCharInfoResult getDsrCharacters(const QString& saveId) const;
    // DS2CharInfoResult getDs2Characters(const QString& saveId) const;
    // DS3CharInfoResult getDs3Characters(const QString& saveId) const;
    // SekiroCharInfoResult getSekiroCharacters(const QString& saveId) const;
    // ERCharInfoResult getERCharacters(const QString& saveId) const;

    void openBackupDir();

private slots:
    void onQuickSaveRequest();
    void onQuickLoadRequest();
    void onHotkeysChange();

private:
    QString m_currentSaveId = "";
    ConfigModel* m_configModel;
    BackupsModel* m_backupsModel;
    HotkeysThread* m_hotkeysThread;
};
