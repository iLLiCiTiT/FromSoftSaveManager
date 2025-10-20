#pragma once
#include <QLabel>
#include <QIcon>
#include <QLabel>
#include <QVBoxLayout>
#include "SquareButton.h"
#include "../parse/Parse.h"

// Tab icon floating label
class TabButtonHint: public QWidget {
    Q_OBJECT
public:
    explicit TabButtonHint(const QString &title, QWidget* parent);
    ~TabButtonHint() override = default;

private:
    QLabel* m_labelWidget;
};

// Tab icon button
class TabIconButton: public SquareButton {
    Q_OBJECT
public:
    explicit TabIconButton(QIcon &icon, const QString &title, QWidget* parent);
    ~TabIconButton() override = default;
    void setSelected(bool selected);
    bool isSelected() const;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    bool m_isSelected = false;
    TabButtonHint* m_hint {nullptr};
};

// Game tab icon button
struct ButtonGameInfo {
    QString title;
    QIcon icon;
};
class GameSaveTabButton: public TabIconButton {
    Q_OBJECT
public:
    explicit GameSaveTabButton(QString save_id, QIcon &icon, const QString &title, QWidget* parent);
    static GameSaveTabButton* fromGame(fsm::parse::Game game, QString save_id, QWidget* parent) {
        ButtonGameInfo gi = getGameInfo(game);
        return new GameSaveTabButton(save_id, gi.icon, gi.title, parent);
    };
private:
    static ButtonGameInfo getGameInfo(fsm::parse::Game game);

    QString m_saveId;
};

// Side bar widget
class SideBarWidget: public QWidget {
    Q_OBJECT
public:
    explicit SideBarWidget(QWidget* parent);

    void addTab(fsm::parse::Game game, QString save_id);
private:
    QVBoxLayout* m_layout {nullptr};
    TabIconButton* m_settingsTab {nullptr};
    std::map<QString, TabIconButton*> m_gameTabs;
    QString m_currentTab;
};