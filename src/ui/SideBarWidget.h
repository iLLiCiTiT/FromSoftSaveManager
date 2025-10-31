#pragma once
#include <QLabel>
#include <QVBoxLayout>
#include "SquareButton.h"
#include "../parse/Parse.h"

// Tab icon floating label
class TabButtonHint: public QWidget {
    Q_OBJECT
public:
    explicit TabButtonHint(const QString& title, QWidget* parent);
    ~TabButtonHint() override = default;
    void setSelected(const bool& selected);

private:
    QLabel* m_labelWidget;
};

// Tab icon button
class TabIconButton: public SquareButton {
    Q_OBJECT
signals:
    void requested(QString saveId);

public:
    explicit TabIconButton(const QIcon& icon, const QString& title, QWidget* parent);
    ~TabIconButton() override = default;
    void setSelected(const bool& selected);
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    bool m_isSelected = false;
    TabButtonHint* m_hint = nullptr;

private slots:
    void virtual onClick();
};

// Game tab icon button
struct ButtonGameInfo {
    QString title;
    QIcon icon;
};
class GameSaveTabButton: public TabIconButton {
    Q_OBJECT
public:
    explicit GameSaveTabButton(const QString& saveId, const QIcon &icon, const QString &title, QWidget* parent);
    static GameSaveTabButton* fromGame(const fsm::parse::Game game, const QString& saveId, QWidget* parent) {
        ButtonGameInfo gi = getGameInfo(game);
        return new GameSaveTabButton(saveId, gi.icon, gi.title, parent);
    };
private:
    QString m_saveId = "";
    static ButtonGameInfo getGameInfo(fsm::parse::Game game);

private slots:
    void onClick() override;
};

// Side bar widget
class SideBarWidget: public QWidget {
    Q_OBJECT
signals:
    void tabChanged(QString saveId);
public:
    explicit SideBarWidget(QWidget* parent);

    void addTab(const fsm::parse::Game& game, const QString& saveId);
    void removeTab(const QString& saveId);
public slots:
    void setCurrentTab(const QString& saveId);

private:
    QVBoxLayout* m_layout {nullptr};
    TabIconButton* m_settingsTab {nullptr};
    std::map<QString, TabIconButton*> m_gameTabs;
    QString m_currentTab;
};