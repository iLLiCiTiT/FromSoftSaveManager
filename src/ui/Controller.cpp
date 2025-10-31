#include "Controller.h"

#include <filesystem>
#include "Config.h"

Controller::Controller(QObject* parent): QObject(parent) {
    m_config = new Config();
};
Controller::~Controller() {
    delete m_config;
};

QString Controller::getLastSelectedSaveId() const {
    return m_config->getLastSelectedSaveId();
}
void Controller::setCurrentTabId(const QString& saveId) {
    m_currentSaveId = saveId;
    m_config->setLastSelectedSaveId(saveId);
}

std::vector<SaveFileItem> Controller::getSaveFileItems() {
    return m_config->getSaveFileItems();
}

DSRCharInfoResult Controller::getDsrCharacters(const QString& saveId) {
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