#include "Controller.h"

#include "Config.h"

Controller::Controller(QObject* parent): QObject(parent) {
    m_config = new Config();
};
Controller::~Controller() {
    delete m_config;
    QObject::~QObject();
};

QString Controller::getLastTabId() const {
    return m_lastTabId;
}
void Controller::setLastTabId(const QString& saveId) {
    m_lastTabId = saveId;
}

std::vector<SaveFileItem> Controller::getSaveFileItems() {
    std::vector<SaveFileItem> output;
    // TODO implement
    output.push_back({ fsm::parse::Game::DSR, "test" });
    return output;
}

DSRCharInfoResult Controller::getDsrCharacters(const QString& saveId) {
    return {
        "Nothing",
        std::vector<fsm::parse::DSRCharacterInfo>()
    };
}