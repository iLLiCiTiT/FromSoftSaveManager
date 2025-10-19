#include "SideBar.h"

#include <QLabel>
#include <QString>
#include <QVBoxLayout>

TabButtonHint::TabButtonHint(const QString &title, QWidget* parent): QWidget(parent) {
    m_labelWidget = new QLabel(title, this);

    QHBoxLayout* m_layout = new QHBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->addWidget(m_labelWidget, 1);

    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
};

TabIconButton::TabIconButton(QIcon &icon, const QString &title, QWidget* parent): SquareButton(parent) {
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

void TabIconButton::enterEvent(QEnterEvent *event) {
    SquareButton::enterEvent(event);
    m_hint->show();
    int y_offset = (height() - m_hint->height()) / 2;
    QPoint pos = mapToGlobal(QPoint(width(), y_offset - 1));
    m_hint->move(pos);
};

void TabIconButton::leaveEvent(QEvent *event) {
    QPushButton::leaveEvent(event);
    m_hint->hide();
};

SideBar::SideBar(QWidget* parent): QWidget(parent) {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    struct TabSpec { const char* icon; const char* title; };
    const TabSpec tabs[] = {
        {":/icons/DSR_256x256.png", "Dark Souls Remastered",},
        {":/icons/DS2-SOTFS_256x256.png", "Dark Souls II: SOTFS"},
        {":/icons/DS3_256x256.png", "Dark Souls III"},
        {":/icons/ER_256x256.png", "Elden Ring"},
        {":/icons/Sekiro_256x256.png", "Sekiro"},
    };
    for (const auto& t : tabs) {
        QIcon icon(t.icon);
        QString title = QString::fromStdString(t.title);
        TabIconButton* btn = new TabIconButton(icon, title, this);
        layout->addWidget(btn, 0);
        m_tabs.push_back(btn);
    }

    layout->addStretch(1);
    QIcon icon(":/icons/settings_256x256.png");
    QString title = "Settings";
    TabIconButton* settingBtn = new TabIconButton(icon, title, this);
    layout->addWidget(settingBtn, 0);
    m_tabs.push_back(settingBtn);
};
