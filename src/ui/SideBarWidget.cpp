#include "SideBarWidget.h"

#include <iostream>
#include <QLabel>
#include <QStyle>
#include <QString>
#include <QVBoxLayout>
#include "../parse/Parse.h"

TabButtonHint::TabButtonHint(const QString& title, QWidget* parent): QWidget(parent) {
    m_labelWidget = new QLabel(title, this);

    QHBoxLayout* m_layout = new QHBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->addWidget(m_labelWidget);

    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
};

void TabButtonHint::setSelected(const bool& selected) {
    m_labelWidget->setProperty("selected", selected ? "1" : "0");
    m_labelWidget->style()->polish(m_labelWidget);
}

TabIconButton::TabIconButton(const QIcon& icon, const QString& title, QWidget* parent): SquareButton(parent) {
    m_hint = new TabButtonHint(title, this);
    connect(this, SIGNAL(clicked()), SLOT(onClick()));
    setIcon(icon);
};

void TabIconButton::setSelected(const bool& selected) {
    if (selected == m_isSelected) return;
    m_isSelected = selected;
    setProperty("selected", selected ? "1" : "0");
    style()->polish(this);
    m_hint->setSelected(selected);
};

void TabIconButton::enterEvent(QEnterEvent* event) {
    SquareButton::enterEvent(event);
    m_hint->show();
    int y_offset = (height() - m_hint->height()) / 2;
    QPoint pos = mapToGlobal(QPoint(width(), y_offset - 1));
    m_hint->move(pos);
};

void TabIconButton::leaveEvent(QEvent* event) {
    QPushButton::leaveEvent(event);
    m_hint->hide();
};

void TabIconButton::onClick() {
    emit requested("");
};

GameSaveTabButton::GameSaveTabButton(const QString& save_id, const QIcon &icon, const QString &title, QWidget* parent): TabIconButton(icon, title, parent) {
    m_saveId = QString(save_id);
}

ButtonGameInfo GameSaveTabButton::getGameInfo(fsm::parse::Game game) {
    QString title;
    switch (game) {
        case fsm::parse::Game::DSR:
            return {"DS: Remastered", QIcon(":/icons/DSR_256x256.png")};
        case fsm::parse::Game::DS2_SOTFS:
            return {"DS II: SotFS", QIcon(":/icons/DS2-SOTFS_256x256.png")};
        case fsm::parse::Game::DS3:
            return {"Dark Souls III", QIcon(":/icons/DS3_256x256.png")};
        case fsm::parse::Game::Sekiro:
            return {"Sekiro: Shadows Die Twice", QIcon(":/icons/ER_256x256.png")};
        case fsm::parse::Game::ER:
            return {"Elden Ring", QIcon(":/icons/Sekiro_256x256.png")};
        default:
            return {"Unknown", QIcon()};
    }
};

void GameSaveTabButton::onClick() {
    emit requested(m_saveId);
};

SideBarWidget::SideBarWidget(QWidget* parent): QWidget(parent) {
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(4, 4, 4, 4);
    m_layout->setSpacing(3);
    m_layout->addStretch(1);

    QIcon icon(":/icons/settings_256x256.png");
    QString title = "Settings";
    m_settingsTab = new TabIconButton(icon, title, this);
    m_settingsTab->setSelected(true);
    m_layout->addWidget(m_settingsTab, 0);

    connect(m_settingsTab, SIGNAL(requested(QString)), this, SLOT(setCurrentTab(QString)));
};

void SideBarWidget::addTab(const fsm::parse::Game& game, const QString& save_id) {
    GameSaveTabButton* tab_btn = GameSaveTabButton::fromGame(game, save_id, this);
    m_gameTabs[save_id] = tab_btn;
    connect(tab_btn, SIGNAL(requested(QString)), this, SLOT(setCurrentTab(QString)));
    m_layout->insertWidget(m_layout->count() - 2, tab_btn, 0);
    if (m_currentTab.isEmpty()) {
        setCurrentTab(save_id);
    }
};

void SideBarWidget::removeTab(const QString& save_id) {
    if (save_id == m_currentTab) setCurrentTab("");
    if (m_gameTabs.find(save_id) == m_gameTabs.end()) return;

    TabIconButton* btn = m_gameTabs.at(save_id);

    btn->setVisible(false);
    if (const int idx = m_layout->indexOf(btn); idx != -1) m_layout->takeAt(idx);
    btn->deleteLater();
};

void SideBarWidget::setCurrentTab(const QString& save_id) {
    if (save_id == m_currentTab) {
        return;
    }
    if (save_id.isEmpty())
        m_settingsTab->setSelected(true);
    else
        m_gameTabs.at(save_id)->setSelected(true);

    if (m_currentTab.isEmpty())
        m_settingsTab->setSelected(false);
    else
        m_gameTabs.at(m_currentTab)->setSelected(false);
    m_currentTab = QString(save_id);
    emit tabChanged(save_id);
};