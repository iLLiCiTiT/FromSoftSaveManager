#pragma once

#include <QSettings>
#include <QWidget>
#include <QKeyEvent>

#include "../parse/DSRSaveFile.h"

struct DSRCharInfoResult {
    QString error;
    std::vector<fsm::parse::DSRCharacterInfo> characters;
};

class Controller : public QObject {
    Q_OBJECT
public:
    explicit Controller(QObject* parent = nullptr)
        : QObject(parent)
        , m_settings("FromSoft", "FromSoftSaveManager") {
        loadConfig();
    }
    ~Controller() override = default;

    // Configuration API
    int lastTabIndex() const { return m_lastTabIndex; }
    void setLastTabIndex(int idx) { m_lastTabIndex = idx; }

    void save_config() {
        // Persist minimal config (extend as needed)
        m_settings.setValue(QStringLiteral("ui/lastTabIndex"), m_lastTabIndex);
        m_settings.sync();
    }

    DSRCharInfoResult getDsrCharacters(const QString& m_saveId) {
        return {
            "Nothing",
            std::vector<fsm::parse::DSRCharacterInfo>()
        };
    };

signals:
    void quicksaveRequested();
    void quickloadRequested();
    void openRequested();
    void refreshRequested();

private:
    void loadConfig() {
        m_lastTabIndex = m_settings.value(QStringLiteral("ui/lastTabIndex"), 0).toInt();
    }

private:
    QSettings m_settings;
    int m_lastTabIndex {0};
};
