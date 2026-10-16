#pragma once

#include <QWidget>
#include <vector>

#include "Painting.h"
#include "../../parse/Sekiro/SaveFile.h"

namespace fssm::ui::sekiro {
// Inventory screen drawn with the game's own menu art: page tabs, category
// icons, a list of ink strokes with item icons and counts, the item detail
// panel and the status bar (Vitality, Attack Power, sen).
class InventoryWidget: public QWidget {
    Q_OBJECT
public:
    explicit InventoryWidget(QWidget* parent = nullptr);
    void setCharacter(const fssm::parse::sekiro::SekiroCharacterInfo* charInfo);
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    struct Layout {
        double scale = 1.0;
        std::vector<QRect> tabs;
        QRect categoryLabel;
        std::vector<QRect> categories;
        QRect list;
        int rowHeight = 0;
        QRect scrollTrack;
        QRect detail;
        QRect status;
    };

    Layout computeLayout() const;
    void rebuildVisibleItems();
    int maxScroll() const;
    int itemIndexAt(const QPoint& pos) const;
    void selectPage(int page, int category);
    void drawTabs(QPainter& painter);
    void drawCategories(QPainter& painter);
    void drawList(QPainter& painter);
    void drawDetail(QPainter& painter);
    void drawStatus(QPainter& painter);
    QPixmap itemIcon(const fssm::parse::sekiro::InventoryItem& item, int size);

    std::vector<fssm::parse::sekiro::InventoryItem> m_allItems;
    std::vector<const fssm::parse::sekiro::InventoryItem*> m_visibleItems;
    fssm::parse::sekiro::SekiroCharacterInfo m_stats;   // without items
    bool m_hasCharacter = false;
    int m_page = 0;                 // index into PAGES
    int m_category = 0;             // index into the page's categories
    int m_scroll = 0;               // pixels
    int m_hoverItem = -1;
    int m_selectedItem = -1;

    Layout m_layout;
    PixmapCache m_cache;
};
}
