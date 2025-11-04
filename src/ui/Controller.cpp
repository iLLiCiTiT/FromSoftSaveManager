#include "Controller.h"

#include <QDesktopServices>
#include <QUrl>
#include <filesystem>
#include <iostream>


HotkeysThread::HotkeysThread(const ConfigHotkeys& config, QObject* parent): QThread(parent) {
    updateHotkeys(config);
}

void HotkeysThread::stop() {
    m_isRunning = false;
}

void HotkeysThread::run() {
    m_isRunning = true;
    bool quickSavePressed = false;
    bool quickLoadPressed = false;
    while (m_isRunning) {
        bool triggerSave = false;
        bool triggerLoad = false;
        if (m_quickSaveHotkey.empty() || keysArePressed(m_quickSaveHotkey)) {
            quickSavePressed = true;
        } else if (quickSavePressed) {
            quickSavePressed = false;
            triggerSave = true;
        }

        if (m_quickLoadHotkey.empty() || keysArePressed(m_quickLoadHotkey)) {
            quickLoadPressed = true;
        } else if (quickLoadPressed) {
            quickLoadPressed = false;
            triggerLoad = true;
        }

        if (m_hotkeysChanged) {
            m_hotkeysChanged = false;
        } else if (triggerSave) {
            emit quickSaveRequested();
        } else if (triggerLoad) {
            emit quickLoadRequested();
        }
        msleep(10);
    }
}

void HotkeysThread::updateHotkeys(const ConfigHotkeys& hotkeys) {
    m_quickSaveHotkey = qtCombinationToInt(hotkeys.quickSaveHotkey);
    m_quickLoadHotkey = qtCombinationToInt(hotkeys.quickLoadHotkey);
    m_hotkeysChanged = true;
}

// --- SaveChangesThread ---
std::filesystem::file_time_type getFileModificationTime(const std::filesystem::path& filePath) {
    if (!std::filesystem::exists(filePath)) return std::filesystem::file_time_type {};
    return std::filesystem::last_write_time(filePath);
}

SaveChangesThread::SaveChangesThread(const std::vector<SaveFileItem>& saveItems, QObject* parent) : QThread(parent) {
    updatePaths(saveItems);
}

void SaveChangesThread::updatePaths(const std::vector<SaveFileItem>& saveItems) {
    std::unordered_set<QString> currentIds;
    for (auto& [saveId, _]: m_saveFilesBySaveId) {
        currentIds.insert(saveId);
    }
    for (auto& saveItem: saveItems) {
        if (currentIds.find(saveItem.saveId) != currentIds.end()) {
            currentIds.erase(saveItem.saveId);
        }
        std::filesystem::path path = saveItem.savePath.toStdString();
        m_saveFilesBySaveId[saveItem.saveId] = path;
        m_lastChangedById[saveItem.saveId] = getFileModificationTime(path);
    }
    for (auto& saveId: currentIds) {
        m_saveFilesBySaveId.erase(saveId);
        m_lastChangedById.erase(saveId);
    }
}

void SaveChangesThread::stop() {
    m_isRunning = false;
}

void SaveChangesThread::run() {
    m_isRunning = true;

    while (m_isRunning) {
        for (auto& [saveId, path]: m_saveFilesBySaveId) {
            std::filesystem::file_time_type& oldMod = m_lastChangedById[saveId];
            std::filesystem::file_time_type newMod = getFileModificationTime(path);
            if (oldMod == newMod) continue;
            m_lastChangedById[saveId] = newMod;
            emit saveFileChanged(saveId);
        }
        msleep(1000);
    }
}

// --- Controller ---
Controller::Controller(QObject* parent): QObject(parent) {
    m_configModel = new ConfigModel(this);
    auto saveFileItems = m_configModel->getSaveFileItems();
    m_backupsModel = new BackupsModel(saveFileItems, m_configModel->getAutosaveConfig(), m_configModel->getBackupDirPath(), this);
    m_hotkeysThread = new HotkeysThread(m_configModel->getHotkeysConfig(), this);
    m_saveChangesThread = new SaveChangesThread(saveFileItems, this);

    connect(m_configModel, SIGNAL(pathsChanged()), this, SLOT(onGamePathsChange()));
    connect(m_configModel, SIGNAL(hotkeysChanged()), this, SLOT(onHotkeysChange()));
    connect(m_configModel, SIGNAL(autoBackupChanged()), this, SLOT(onAutobackupChange()));

    connect(m_hotkeysThread, SIGNAL(quickSaveRequested()), this, SLOT(onQuickSaveRequest()));
    connect(m_hotkeysThread, SIGNAL(quickLoadRequested()), this, SLOT(onQuickLoadRequest()));

    connect(m_saveChangesThread, SIGNAL(saveFileChanged(QString)), this, SLOT(onSaveFileChange(QString)));

    m_hotkeysThread->start();
    m_saveChangesThread->start();
}

Controller::~Controller() {
    delete m_configModel;
    m_hotkeysThread->stop();
    m_hotkeysThread->wait();
    m_hotkeysThread->deleteLater();
    m_saveChangesThread->stop();
    m_saveChangesThread->wait();
    m_saveChangesThread->deleteLater();
}

QString Controller::getLastSelectedSaveId() const {
    return m_configModel->getLastSelectedSaveId();
}

void Controller::setCurrentTabId(const QString& saveId) {
    m_currentSaveId = saveId;
    m_configModel->setLastSelectedSaveId(saveId);
}

ConfigSettingsData Controller::getConfigSettingsData() const {
    return m_configModel->getConfigSettingsData();
}

ConfigHotkeys Controller::getHotkeysConfig() const {
    return m_configModel->getHotkeysConfig();
}

void Controller::saveConfigData(const ConfigConfirmData& confirmData) {
    m_configModel->saveConfigData(confirmData);
}

std::vector<SaveFileItem> Controller::getSaveFileItems() const {
    return m_configModel->getSaveFileItems();
}

DSRCharInfoResult Controller::getDsrCharacters(const QString& saveId) const {
    QString r_savePath = m_configModel->getSavePathItem(saveId);
    if (r_savePath.isEmpty()) return {
        "Save file path is not set.",
        std::vector<fssm::parse::DSRCharacterInfo> {},
    };
    std::string savePath = r_savePath.toStdString();
    if (!std::filesystem::exists(savePath)) return {
        "Save file does not exist.",
        std::vector<fssm::parse::DSRCharacterInfo> {},
    };
    fssm::parse::SL2File sl2_dsr = fssm::parse::parse_sl2_file(savePath);
    fssm::parse::DSRSaveFile dsr = fsm::parse::parse_dsr_file(sl2_dsr);

    return {
        "",
        dsr.characters
    };
}

void Controller::openBackupDir() {
    if (m_currentSaveId.isEmpty()) return;
    auto itemOpt = m_configModel->getSaveItem(m_currentSaveId);
    if (!itemOpt.has_value()) return;
    std::string backupDir = m_backupsModel->getGameBackupDir(itemOpt.value().game);
    if (!std::filesystem::exists(backupDir)) return;
    QDesktopServices::openUrl(QUrl::fromLocalFile(QString::fromStdString(backupDir)));
}

void Controller::onQuickSaveRequest() {
    if (m_currentSaveId.isEmpty()) return;
    auto itemOpt = m_configModel->getSaveItem(m_currentSaveId);
    if (!itemOpt.has_value()) return;
    QString savePath = itemOpt.value().savePath;
    if (savePath.isEmpty()) return;
    m_backupsModel->createQuickSaveBackup(savePath, itemOpt.value().game);
};

void Controller::onQuickLoadRequest() {
    if (m_currentSaveId.isEmpty()) return;
    auto itemOpt = m_configModel->getSaveItem(m_currentSaveId);
    if (!itemOpt.has_value()) return;
    m_backupsModel->quickLoad(itemOpt.value().savePath, itemOpt.value().game);
}

void Controller::createManualBackup(const QString& label) {
    if (m_currentSaveId.isEmpty()) return;
    auto itemOpt = m_configModel->getSaveItem(m_currentSaveId);
    if (!itemOpt.has_value()) return;
    m_backupsModel->createManualBackup(itemOpt.value().savePath, itemOpt.value().game, label);
}

void Controller::deleteBackupByIds(const std::vector<QString>& backupIds) {
    if (m_currentSaveId.isEmpty()) return;
    auto itemOpt = m_configModel->getSaveItem(m_currentSaveId);
    if (!itemOpt.has_value()) return;
    m_backupsModel->deleteBackupByIds(itemOpt.value().game, backupIds);
}

// Config changed slots
void Controller::onGamePathsChange() {
    m_saveChangesThread->updatePaths(m_configModel->getSaveFileItems());
    emit pathsConfigChanged();
}

void Controller::onHotkeysChange() {
    m_hotkeysThread->updateHotkeys(m_configModel->getHotkeysConfig());
    emit hotkeysConfigChanged();
}

void Controller::onAutobackupChange() {
    m_backupsModel->updateAutobackupConfig(m_configModel->getAutosaveConfig());
    emit autobackupConfigChanged();
}

// Save file changed
void Controller::onSaveFileChange(const QString& saveId) {
    m_backupsModel->saveGameChanged(saveId);
    emit saveIdChanged(saveId);
}
