#pragma once

#include <QPixmap>
#include <QWidget>
#include <vector>

class QStackedWidget;

namespace fssm::ui::ds3 {
// Tab switcher drawn like the game's menu headers ("Inventory" with its icon):
// the current tab is bright with a gold rule, the others are dimmed. Pages are
// shown below the header.
class HeaderTabs: public QWidget {
    Q_OBJECT
public:
    explicit HeaderTabs(QWidget* parent);
    void addTab(const QString& title, const QString& icon, QWidget* page);
    void addTab(const QString& title, const QPixmap& icon, QWidget* page);
    void setCurrentIndex(int index);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    struct Tab {
        QString title;
        QPixmap icon;
    };

    // Width of a tab with its title, or of its icon alone.
    int tabWidth(int index, bool iconOnly) const;
    // The titles don't fit: the tabs other than the current one show only their icon
    // (and the title as a tooltip), like the game's own menu headers.
    bool compact() const;
    QRect tabRect(int index) const;
    int tabAt(const QPoint& pos) const;

    QStackedWidget* m_stack;
    std::vector<Tab> m_tabs;
    int m_current = -1;
    int m_hover = -1;
};
}
