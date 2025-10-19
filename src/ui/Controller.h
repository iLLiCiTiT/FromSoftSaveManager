#pragma once

#include <QObject>
#include <QSettings>
#include <QKeySequence>
#include <QWidget>
#include <QEvent>
#include <QKeyEvent>

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

    // Install as global event filter to handle key presses
    bool eventFilter(QObject* obj, QEvent* event) override;

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

// Inline implementation of the event filter
inline bool Controller::eventFilter(QObject* obj, QEvent* event) {
    // Q_UNUSED(obj);
    // if (event->type() == QEvent::KeyPress) {
    //     auto* ke = static_cast<QKeyEvent*>(event);
    //     const Qt::KeyboardModifiers mods = ke->modifiers();
    //     const int key = ke->key();
    //     if ((mods & Qt::ControlModifier) && key == Qt::Key_O) {
    //         emit openRequested();
    //         return false;
    //     }
    //     if ((mods & Qt::ControlModifier) && key == Qt::Key_S) {
    //         emit quicksaveRequested();
    //         return false;
    //     }
    //     if ((mods & Qt::ControlModifier) && key == Qt::Key_L) {
    //         emit quickloadRequested();
    //         return false;
    //     }
    //     if (mods == Qt::NoModifier && key == Qt::Key_F5) {
    //         emit refreshRequested();
    //         return false;
    //     }
    // }
    return QObject::eventFilter(obj, event);
}
