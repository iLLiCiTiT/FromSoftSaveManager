#include "Controller.h"

#include "Config.h"

Controller::Controller(QObject* parent): QObject(parent) {
    m_config = new Config();
};
Controller::~Controller() {
    delete m_config;
    QObject::~QObject();
};

QString Controller::getCurrentTabId() const {
    return m_currentSaveId;
}
void Controller::setCurrentTabId(const QString& saveId) {
    m_currentSaveId = saveId;
    m_config->setLastSelectedSaveId(saveId);
}

std::vector<SaveFileItem> Controller::getSaveFileItems() {
    return m_config->getSaveFileItems();
}

DSRCharInfoResult Controller::getDsrCharacters(const QString& saveId) {
    return {
        "Nothing",
        std::vector<fsm::parse::DSRCharacterInfo>()
    };
}