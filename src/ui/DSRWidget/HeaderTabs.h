#pragma once

#include <QPixmap>
#include <QWidget>
#include <vector>

class QStackedWidget;

namespace fssm::ui::dsr {
// Tab switcher drawn like the game's menu header: the current page's framed icon with
// its title and subtitle on the left, and a row of small screen icons on the right
// (in colour for the current page, grey for the others) to switch pages. Pages are
// shown below the header.
class HeaderTabs: public QWidget {
    Q_OBJECT
public:
    explicit HeaderTabs(QWidget* parent);
    void addTab(const QString& title, const QString& subtitle, const QPixmap& icon, QWidget* page);
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
        QString subtitle;
        QPixmap icon;
        QPixmap greyIcon;
    };

    QRect iconRect(int index) const;
    int tabAt(const QPoint& pos) const;

    QStackedWidget* m_stack;
    std::vector<Tab> m_tabs;
    int m_current = -1;
    int m_hover = -1;
};
}
