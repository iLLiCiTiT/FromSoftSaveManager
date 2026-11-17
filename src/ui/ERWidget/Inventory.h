#pragma once

#include <array>

#include <QHash>
#include <QPixmap>
#include <QWidget>
#include <vector>

#include "../ScrollHelper.h"
#include "../../parse/EldenRing/SaveFile.h"

namespace fssm::ui::er {
// Inventory screen drawn like the game's: category tabs, an item grid split
// into sort groups by separator lines (as many columns as fit, at least the
// game's five) and a strip under it with the selected item's details.
class InventoryWidget: public QWidget {
    Q_OBJECT
public:
    explicit InventoryWidget(QWidget* parent = nullptr);
    void setCharacter(const fssm::parse::er::ERCharacterInfo* charInfo);
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    struct Entry {
        fssm::parse::er::InventoryItem item;
        uint32_t stored = 0;        // same item in the storage chest (stacks), or this piece
        bool held = true;           // false when kept only in the storage chest
        int upgradeLevel = 0;       // weapons
        QString title;
    };

    // A grid row; a new sort group always starts on a new row.
    struct Row {
        int first = 0;              // index into Pane::visible
        int count = 0;
        bool groupStart = false;    // separator line above (not on the first row)
        int top = 0;                // y offset inside the grid content
    };

    struct Layout {
        double scale = 1.0;
        QRect panel;
        QRect tabs;
        int tabWidth = 0;
        int visibleTabs = 5;
        QRect leftButton;
        QRect rightButton;
        std::array<QRect, 2> caption;     // "Inventory" / "Sort Chest" above each grid
        std::array<QRect, 2> grid;
        int dividerX = 0;                 // the line between the two grids
        std::array<QRect, 2> scrollTrack; // fixed at each grid's right edge
        std::array<int, 2> columns {5, 5};
        int cellWidth = 0;
        int rowHeight = 0;
        int separatorGap = 0;
        QRect detail;               // selected item, under the grid
    };

    Layout computeLayout() const;
    void rebuildVisibleItems();
    void layoutRows();              // both panes
    void selectCategory(int category);
    void keepCategoryVisible();
    void selectItem(int index);     // in the active pane
    int contentHeight(int pane) const;
    void updateScrollRange();
    // Item under pos and its pane, or -1.
    int itemIndexAt(const QPoint& pos, int* pane) const;
    void setHover(const QPoint& pos);
    int tabIndexAt(const QPoint& pos) const;
    QRect cellRect(int pane, int row, int column) const;
    const Entry* currentEntry() const;
    QPixmap scaled(const QString& resource, const QSize& size);
    QPixmap itemIcon(const Entry& entry, const QSize& size);
    void drawTabs(QPainter& painter);
    void drawGrid(QPainter& painter, int pane);
    void drawDetail(QPainter& painter);

    // One item grid: HELD_PANE (carried) or STORAGE_PANE (the sort chest).
    struct Pane {
        explicit Pane(QWidget* owner): scroll(owner) {}
        std::vector<const Entry*> visible;
        std::vector<Row> rows;
        fssm::ui::PaintedScroll scroll;     // range kept by updateScrollRange()
        int hover = -1;
        int selected = 0;
    };
    static constexpr int HELD_PANE = 0;
    static constexpr int STORAGE_PANE = 1;

    std::vector<Entry> m_entries;
    bool m_hasCharacter = false;
    int m_category = 0;             // index into the tab list
    int m_tabOffset = 0;            // first visible tab
    std::array<Pane, 2> m_panes {Pane(this), Pane(this)};
    int m_activePane = HELD_PANE;   // where the last click selected an item
    int m_hoverTab = -1;            // index into the tab list

    Layout m_layout;
    QHash<QString, QPixmap> m_scaledCache;
};
}
