#include "SideBarWidget.h"

#include <QLabel>
#include <QString>
#include <QVBoxLayout>

TabButtonHint::TabButtonHint(const QString& title, QWidget* parent): QWidget(parent) {
    m_labelWidget = new QLabel(title, this);

    QHBoxLayout* m_layout = new QHBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->addWidget(m_labelWidget);

    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
};

TabIconButton::TabIconButton(const QIcon& icon, const QString& title, QWidget* parent): SquareButton(parent) {
    m_hint = new TabButtonHint(title, this);
    setIcon(icon);
};

void TabIconButton::setSelected(bool selected) {
    if (selected == m_isSelected) return;
    m_isSelected = selected;
    // TODO change style
};

bool TabIconButton::isSelected() const {
    return m_isSelected;
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

GameSaveTabButton::GameSaveTabButton(QString save_id, QIcon &icon, const QString &title, QWidget* parent): TabIconButton(icon, title, parent) {
    m_saveId = save_id;
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

SideBarWidget::SideBarWidget(QWidget* parent): QWidget(parent) {
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(4, 4, 4, 4);
    m_layout->setSpacing(3);
    m_layout->addStretch(1);

    QIcon icon(":/icons/settings_256x256.png");
    QString title = "Settings";
    m_settingsTab = new TabIconButton(icon, title, this);
    m_layout->addWidget(m_settingsTab, 0);
};

void SideBarWidget::addTab(fsm::parse::Game game, QString save_id) {
    GameSaveTabButton* tab_btn = GameSaveTabButton::fromGame(game, save_id, this);
    m_gameTabs[save_id] = tab_btn;
    // // tab_btn.requested.connect(self.setCurrentTab);
    m_layout->insertWidget(m_layout->count() - 2, tab_btn, 0);
};
