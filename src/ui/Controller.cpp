#include "Controller.h"

#include <filesystem>
#include <iostream>

#include "Config.h"


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
    m_config = new Config();
    m_hotkeysThread = new HotkeysThread(m_config->getHotkeysConfig(), this);

    connect(m_hotkeysThread, SIGNAL(quickSaveRequested()), this, SLOT(onQuickSaveRequest()));
    connect(m_hotkeysThread, SIGNAL(quickLoadRequested()), this, SLOT(onQuickLoadRequest()));

    m_hotkeysThread->start();
};
Controller::~Controller() {
    delete m_config;
    m_hotkeysThread->stop();
    m_hotkeysThread->wait();
    m_hotkeysThread->deleteLater();
}

QString Controller::getLastSelectedSaveId() const {
    return m_config->getLastSelectedSaveId();
}

void Controller::setCurrentTabId(const QString& saveId) {
    m_currentSaveId = saveId;
    m_config->setLastSelectedSaveId(saveId);
}

ConfigSettingsData Controller::getConfigSettingsData() const {
    return m_config->getConfigSettingsData();
}

void Controller::saveConfigData(const ConfigConfirmData& confirmData) {
    m_config->saveConfigData(confirmData);
};

std::vector<SaveFileItem> Controller::getSaveFileItems() const {
    return m_config->getSaveFileItems();
}

DSRCharInfoResult Controller::getDsrCharacters(const QString& saveId) const {
    auto r_savePath = m_config->getSavePathItem(saveId);
    if (!r_savePath.has_value()) return {
        "Save file path is not set.",
        std::vector<fsm::parse::DSRCharacterInfo> {},
    };
    std::string savePath = r_savePath.value().toStdString();
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

void Controller::onHotkeysChange() {
    m_hotkeysThread->updateHotkeys(m_config->getHotkeysConfig());
    emit hotkeysChanged();
};

void Controller::onQuickSaveRequest() {
    // TODO implement
    std::cout << "Quick Save Request" << std::endl;
};

void Controller::onQuickLoadRequest() {
    // TODO implement
    std::cout << "Quick Load Request" << std::endl;
};
