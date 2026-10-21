#pragma once

#include <QPixmap>
#include <QWidget>
#include <vector>

class QStackedWidget;

namespace fssm::ui::er {
// Tab switcher drawn like the header of the game's menus (icon and title over a dark
// band, with the gold title line under the current one); the other tabs are dimmed.
// Pages are shown below the header.
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

    QRect tabRect(int index) const;
    int tabAt(const QPoint& pos) const;

    QStackedWidget* m_stack;
    QPixmap m_titleLine;
    std::vector<Tab> m_tabs;
    int m_current = -1;
    int m_hover = -1;
};
}
