#pragma once

#include <QObject>

#include "../parse/DSRSaveFile.h"


struct DSRCharInfoResult {
    QString error;
    std::vector<fsm::parse::DSRCharacterInfo> characters;
};

struct SaveFileItem {
    fsm::parse::Game game;
    QString saveId;
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
    ~Controller() override = default;

    QString getLastTabId() const;
    void setLastTabId(QString saveId);

    std::vector<SaveFileItem> getSaveFileItems();
    DSRCharInfoResult getDsrCharacters(const QString& saveId);

private:
    QString m_lastTabId = "";
};
