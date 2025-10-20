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
    void virtual requested(QString save_id);

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
    explicit GameSaveTabButton(const QString& save_id, const QIcon &icon, const QString &title, QWidget* parent);
    static GameSaveTabButton* fromGame(const fsm::parse::Game game, const QString& save_id, QWidget* parent) {
        ButtonGameInfo gi = getGameInfo(game);
        return new GameSaveTabButton(save_id, gi.icon, gi.title, parent);
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
    void tabChanged(QString save_id);
public:
    explicit SideBarWidget(QWidget* parent);

    void addTab(const fsm::parse::Game& game, const QString& save_id);
    void removeTab(const QString& save_id);
public slots:
    void setCurrentTab(const QString& save_id);

private:
    QVBoxLayout* m_layout {nullptr};
    TabIconButton* m_settingsTab {nullptr};
    std::map<QString, TabIconButton*> m_gameTabs;
    QString m_currentTab;
};