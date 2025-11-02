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

Controller::Controller(QObject* parent): QObject(parent) {
    m_configModel = new ConfigModel(this);
    ConfigAutobackup autosave = m_configModel->getAutosaveConfig();
    m_backupsModel = new BackupsModel(m_configModel->getBackupDirPath(), autosave.maxBackups, this);
    m_hotkeysThread = new HotkeysThread(m_configModel->getHotkeysConfig(), this);

    connect(m_hotkeysThread, SIGNAL(quickSaveRequested()), this, SLOT(onQuickSaveRequest()));
    connect(m_hotkeysThread, SIGNAL(quickLoadRequested()), this, SLOT(onQuickLoadRequest()));

    m_hotkeysThread->start();
}

Controller::~Controller() {
    delete m_configModel;
    m_hotkeysThread->stop();
    m_hotkeysThread->wait();
    m_hotkeysThread->deleteLater();
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
        std::vector<fsm::parse::DSRCharacterInfo> {},
    };
    std::string savePath = r_savePath.toStdString();
    if (!std::filesystem::exists(savePath)) return {
        "Save file does not exist.",
        std::vector<fsm::parse::DSRCharacterInfo> {},
    };
    fsm::parse::SL2File sl2_dsr = fsm::parse::parse_sl2_file(savePath);
    fsm::parse::DSRSaveFile dsr = fsm::parse::parse_dsr_file(sl2_dsr);

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

void Controller::onHotkeysChange() {
    m_hotkeysThread->updateHotkeys(m_configModel->getHotkeysConfig());
    emit hotkeysChanged();
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

void Controller::deleteBackupByIds(const std::vector<QString>& backupIds) {
    if (m_currentSaveId.isEmpty()) return;
    auto itemOpt = m_configModel->getSaveItem(m_currentSaveId);
    if (!itemOpt.has_value()) return;
    m_backupsModel->deleteBackupByIds(itemOpt.value().game, backupIds);
}
