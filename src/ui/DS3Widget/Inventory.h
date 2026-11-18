#pragma once

#include <array>

#include <QHash>
#include <QPixmap>
#include <QWidget>
#include <vector>

#include "../ScrollHelper.h"
#include "../../parse/DS3/SaveFile.h"

namespace fssm::ui::ds3 {
// Inventory screen drawn like the game's storage screen: category tabs above two
// grids of items standing on dishes (carried items left, the storage box right;
// counts, red seals on equipped items), with the selected item's details below.
class InventoryWidget: public QWidget {
    Q_OBJECT
public:
    explicit InventoryWidget(QWidget* parent = nullptr);
    void setCharacter(const fssm::parse::ds3::DS3CharacterInfo* charInfo);
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    struct Entry {
        fssm::parse::ds3::InventoryItem item;
        uint32_t stored = 0;        // same item in the storage box
        bool held = true;           // false when kept only in the storage box
        uint32_t maxHeld = 0;       // 0 for items that don't stack
        QString title;
        QString image;
    };

    struct Layout {
        double scale = 1.0;
        QRect panel;
        QRect leftArrow;
        QRect rightArrow;
        QRect tabs;
        int tabWidth = 0;
        int visibleTabs = 5;
        std::array<QRect, 2> caption;     // "Inventory" / "Storage Box" above each grid
        QRect gridFrame;                  // area of both grids, ruled above and below
        int dividerX = 0;                 // the line between the two grids
        std::array<QRect, 2> half;        // each grid's part of the frame
        std::array<QRect, 2> grid;
        std::array<QRect, 2> scrollTrack; // fixed at each frame's right edge
        std::array<int, 2> columns {5, 5};
        int cellWidth = 0;
        int rowHeight = 0;
        QRect detail;               // selected item, below the grid
    };

    Layout computeLayout() const;
    void rebuildVisibleItems();
    void selectCategory(int category);
    void keepCategoryVisible();
    void updateScrollRange();
    // Item under pos and its pane, or -1.
    int itemIndexAt(const QPoint& pos, int* pane) const;
    int tabIndexAt(const QPoint& pos) const;
    void setHover(const QPoint& pos);
    const Entry* currentEntry() const;
    QPixmap scaled(const QString& resource, const QSize& size);
    QPixmap itemIcon(const Entry& entry, const QSize& size);
    void drawCategories(QPainter& painter);
    void drawFrameRules(QPainter& painter);
    void drawGrid(QPainter& painter, int pane);
    void drawDetail(QPainter& painter);

    // One item grid: HELD_PANE (carried) or STORAGE_PANE (the storage box).
    struct Pane {
        explicit Pane(QWidget* owner): scroll(owner) {}
        std::vector<const Entry*> visible;
        fssm::ui::PaintedScroll scroll;     // range kept by updateScrollRange()
        int hover = -1;
        int selected = 0;
    };
    static constexpr int HELD_PANE = 0;
    static constexpr int STORAGE_PANE = 1;

    std::vector<Entry> m_entries;
    bool m_hasCharacter = false;
    int m_category = 0;             // index into CATEGORY_ORDER
    int m_tabOffset = 0;            // first visible tab
    std::array<Pane, 2> m_panes {Pane(this), Pane(this)};
    int m_activePane = HELD_PANE;   // where the last click selected an item
    int m_hoverTab = -1;            // index into CATEGORY_ORDER

    Layout m_layout;
    QHash<QString, QPixmap> m_scaledCache;
    QPixmap m_tabBackground;
};
}
