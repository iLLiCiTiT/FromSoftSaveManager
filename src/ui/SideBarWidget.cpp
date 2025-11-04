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
    m_hint->show();
    int y_offset = (height() - m_hint->height()) / 2;
    QPoint pos = mapToGlobal(QPoint(width(), y_offset - 1));
    m_hint->move(pos);
};

void TabIconButton::leaveEvent(QEvent* event) {
    m_hint->hide();
};

void TabIconButton::onClick() {
    emit requested("");
};

GameSaveTabButton::GameSaveTabButton(const QString& saveId, const QIcon &icon, const QString &title, QWidget* parent): TabIconButton(icon, title, parent) {
    m_saveId = QString(saveId);
}

ButtonGameInfo GameSaveTabButton::getGameInfo(fssm::Game game) {
    QString title;
    switch (game) {
        case fssm::Game::DSR:
            return {"DS: Remastered", QIcon(":/icons/DSR_256x256.png")};
        case fssm::Game::DS2_SOTFS:
            return {"DS II: SotFS", QIcon(":/icons/DS2-SOTFS_256x256.png")};
        case fssm::Game::DS3:
            return {"Dark Souls III", QIcon(":/icons/DS3_256x256.png")};
        case fssm::Game::Sekiro:
            return {"Sekiro: Shadows Die Twice", QIcon(":/icons/ER_256x256.png")};
        case fssm::Game::ER:
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

void SideBarWidget::addTab(const fssm::Game& game, const QString& saveId) {
    GameSaveTabButton* tab_btn = GameSaveTabButton::fromGame(game, saveId, this);
    m_gameTabs[saveId] = tab_btn;
    connect(tab_btn, SIGNAL(requested(QString)), this, SLOT(setCurrentTab(QString)));
    m_layout->insertWidget(m_layout->count() - 2, tab_btn, 0);
    if (m_currentTab.isEmpty()) {
        setCurrentTab(saveId);
    }
};

void SideBarWidget::removeTab(const QString& saveId) {
    if (saveId == m_currentTab) setCurrentTab("");
    if (m_gameTabs.find(saveId) == m_gameTabs.end()) return;

    TabIconButton* btn = m_gameTabs.at(saveId);

    btn->setVisible(false);
    if (const int idx = m_layout->indexOf(btn); idx != -1) m_layout->takeAt(idx);
    btn->deleteLater();
};

void SideBarWidget::setCurrentTab(const QString& saveId) {
    if (saveId == m_currentTab) {
        return;
    }
    if (saveId.isEmpty())
        m_settingsTab->setSelected(true);
    else
        m_gameTabs.at(saveId)->setSelected(true);

    if (m_currentTab.isEmpty())
        m_settingsTab->setSelected(false);
    else
        m_gameTabs.at(m_currentTab)->setSelected(false);
    m_currentTab = QString(saveId);
    emit tabChanged(saveId);
};