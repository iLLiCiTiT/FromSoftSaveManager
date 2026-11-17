#pragma once

#include <array>

#include <QHash>
#include <QPixmap>
#include <QWidget>
#include <vector>

#include "../ScrollHelper.h"
#include "../../parse/DS2/SaveFile.h"

namespace fssm::ui::ds2 {
// Inventory screen drawn with the game's own menu art (window frame, category
// tiles, item tiles and pedestals extracted by tools/ds2_items/gen_items.py).
class InventoryWidget: public QWidget {
    Q_OBJECT
public:
    explicit InventoryWidget(QWidget* parent = nullptr);
    void setCharacter(const fssm::parse::ds2::DS2CharacterInfo* charInfo);
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
    struct Layout {
        double scale = 1.0;
        QRect content;          // inside the window frame
        QRect titleBar;
        QRect leftArrow;
        QRect rightArrow;
        QRect categories;       // area with the category tiles
        int categorySize = 0;
        int visibleCategories = 1;
        std::array<QRect, 2> caption;     // "Held" / "Item Box" above each grid
        std::array<QRect, 2> grid;
        int dividerX = 0;                 // the line between the two grids
        std::array<QRect, 2> scrollTrack; // fixed at each grid's right edge
        int tileSize = 0;
        std::array<int, 2> columns {1, 1};
        int rowHeight = 0;
        QRect nameBar;
    };

    Layout computeLayout() const;
    void rebuildVisibleItems();
    void updateScrollRange();
    // Item under pos and its pane, or -1.
    int itemIndexAt(const QPoint& pos, int* pane) const;
    void setHover(const QPoint& pos);
    int categoryIndexAt(const QPoint& pos) const;
    int maxCategoryOffset() const;
    QPixmap scaled(const QString& resource, const QSize& size);
    void drawFrame(QPainter& painter);
    void drawCategories(QPainter& painter);
    void drawGrid(QPainter& painter, int pane);
    void drawNameBar(QPainter& painter);
    QString itemTitle(const fssm::parse::ds2::InventoryItem& item) const;

    // Item tile pitch in pixels, independent of the widget size (same as the
    // DS3 and Elden Ring inventories); resizing adds or removes columns.
    static constexpr int TILE_SIZE = 60;
    // Frame, categories, name bar and fonts keep one size (the look at the default
    // window size); only the item grid follows the widget size.
    static constexpr double FIXED_SCALE = 0.63;
    static constexpr int CATEGORY_SIZE = 47;

    std::vector<fssm::parse::ds2::InventoryItem> m_allItems;
    // One item grid: HELD_PANE (carried) or STORAGE_PANE (the Item Box).
    struct Pane {
        explicit Pane(QWidget* owner): scroll(owner) {}
        std::vector<const fssm::parse::ds2::InventoryItem*> visible;
        fssm::ui::PaintedScroll scroll;     // range kept by updateScrollRange()
        int hover = -1;
        int selected = -1;
    };
    static constexpr int HELD_PANE = 0;
    static constexpr int STORAGE_PANE = 1;
    bool m_hasCharacter = false;
    int m_category = 0;             // index into CATEGORY_ORDER
    int m_categoryOffset = 0;       // first visible category
    std::array<Pane, 2> m_panes {Pane(this), Pane(this)};
    int m_activePane = HELD_PANE;   // where the last click selected an item
    int m_hoverCategory = -1;       // index into CATEGORY_ORDER

    Layout m_layout;
    QHash<QString, QPixmap> m_scaledCache;
    QPixmap m_window;
};
}
