#pragma once

#include <QObject>

#include "Config.h"
#include "../parse/DSRSaveFile.h"


struct DSRCharInfoResult {
    QString error;
    std::vector<fsm::parse::DSRCharacterInfo> characters;
};

class Controller : public QObject {
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

    QString getCurrentTabId() const;
    void setCurrentTabId(const QString& saveId);

    std::vector<SaveFileItem> getSaveFileItems();
    DSRCharInfoResult getDsrCharacters(const QString& saveId);

private:
    QString m_currentSaveId = "";
    Config* m_config;
};
