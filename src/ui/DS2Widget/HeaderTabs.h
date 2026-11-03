#pragma once

#include <QPixmap>
#include <QWidget>
#include <vector>

class QStackedWidget;

namespace fssm::ui::ds2 {
// Tab switcher drawn like the game's menu: each tab is one of the stone
// medallions from the menu bar (ui_menu_tabs.png) with its title next to it.
// The current tab is lit and its title sits on the amber selection band.
// Pages are shown below the header.
class HeaderTabs: public QWidget {
    Q_OBJECT
public:
    // Medallions in ui_menu_tabs.png, left to right.
    enum class Medallion { Equipment = 0, Inventory, Status, Scroll, Covenant };

    // Height taken by the tab bar above the pages.
    static constexpr int HEADER_HEIGHT = 30;

    explicit HeaderTabs(QWidget* parent);
    void addTab(const QString& title, Medallion icon, QWidget* page);
    void setCurrentIndex(int index);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;
    // Clears the hover when the mouse moves on into the pages: going from this widget
    // into a child sends it no leave event.
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    struct Tab {
        QString title;
        QPixmap icon;
    };

    // Width of a tab with its title, or of its icon alone.
    int tabWidth(int index, bool iconOnly) const;
    // The titles don't fit: the tabs other than the current one show only their icon,
    // like the game's own menu headers; a hovered icon shows its title floating beside it.
    bool compact() const;
    QRect tabRect(int index) const;
    int tabAt(const QPoint& pos) const;

    QStackedWidget* m_stack;
    QPixmap m_medallions;
    QPixmap m_selectedBand;
    std::vector<Tab> m_tabs;
    int m_current = -1;
    int m_hover = -1;
};
}
