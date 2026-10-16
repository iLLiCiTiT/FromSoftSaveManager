#pragma once

#include <QHash>
#include <QPixmap>
#include <QWidget>
#include <vector>

#include "../../parse/DS3/SaveFile.h"

namespace fssm::ui::ds3 {
// Inventory screen drawn like the game's: category tabs above a grid of items
// standing on dishes (counts, red seals on equipped items), with the selected
// item's details below.
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
    void wheelEvent(QWheelEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    struct Entry {
        fssm::parse::ds3::InventoryItem item;
        uint32_t stored = 0;        // same item in the storage box
        uint32_t maxHeld = 0;       // 0 for items that don't stack
        QString title;
        QString image;
    };

    struct Layout {
        double scale = 1.0;
        QRect panel;
        QRect categoryLabel;
        QRect leftArrow;
        QRect rightArrow;
        QRect tabs;
        int tabWidth = 0;
        int visibleTabs = 5;
        QRect gridFrame;
        QRect grid;
        int columns = 5;
        int cellWidth = 0;
        int rowHeight = 0;
        QRect detail;               // selected item, below the grid
    };

    Layout computeLayout() const;
    void rebuildVisibleItems();
    void selectCategory(int category);
    void keepCategoryVisible();
    int maxScroll() const;
    int itemIndexAt(const QPoint& pos) const;
    int tabIndexAt(const QPoint& pos) const;
    const Entry* currentEntry() const;
    QPixmap scaled(const QString& resource, const QSize& size);
    QPixmap itemIcon(const Entry& entry, const QSize& size);
    void drawCategories(QPainter& painter);
    void drawGrid(QPainter& painter);
    void drawDetail(QPainter& painter);

    std::vector<Entry> m_entries;
    std::vector<const Entry*> m_visible;
    bool m_hasCharacter = false;
    int m_category = 0;             // index into CATEGORY_ORDER
    int m_tabOffset = 0;            // first visible tab
    int m_scroll = 0;               // pixels
    int m_hoverItem = -1;
    int m_selectedItem = 0;

    Layout m_layout;
    QHash<QString, QPixmap> m_scaledCache;
    QPixmap m_tabBackground;
};
}
