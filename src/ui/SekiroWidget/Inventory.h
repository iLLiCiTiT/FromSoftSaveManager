#pragma once

#include <QWidget>
#include <unordered_set>
#include <vector>

#include "../ScrollHelper.h"
#include "Painting.h"
#include "WorldProgress.h"
#include "../../parse/Sekiro/SaveFile.h"
#include "../../parse/Sekiro/SkillTree.h"

namespace fssm::ui::sekiro {
// Inventory screen drawn with the game's own menu art: page tabs, category
// icons, a list of ink strokes with item icons and counts, the item detail
// panel and the status bar (Vitality, Attack Power, sen). The Skills tab shows
// the five Acquire Skills trees with the skills the character has learned, the
// World tab the idols found and bosses defeated (WorldProgressWidget).
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
    void mouseReleaseEvent(QMouseEvent* event) override;
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
    void updateScrollRange();
    int itemIndexAt(const QPoint& pos) const;      // list row, or skill node on the Skills tab
    bool skillPage() const;
    bool worldPage() const;
    QPoint nodeCenter(const fssm::parse::sekiro::SkillNode& node, int* nodeSize = nullptr) const;
    bool learned(const fssm::parse::sekiro::SkillNode& node) const;
    void selectPage(int page, int category);
    void drawTabs(QPainter& painter);
    void drawCategories(QPainter& painter);
    void drawList(QPainter& painter);
    void drawSkillTree(QPainter& painter);
    void drawSkillDetail(QPainter& painter, const QRect& inner);
    void drawDetail(QPainter& painter);
    void drawStatus(QPainter& painter);
    QPixmap itemIcon(const fssm::parse::sekiro::InventoryItem& item, int size);
    QPixmap skillIcon(const fssm::parse::sekiro::SkillNode& node, int size);
    void drawSkillNode(QPainter& painter, const fssm::parse::sekiro::SkillNode& node, const QRect& box);

    std::vector<fssm::parse::sekiro::InventoryItem> m_allItems;
    std::vector<const fssm::parse::sekiro::InventoryItem*> m_visibleItems;
    std::vector<const fssm::parse::sekiro::SkillNode*> m_treeNodes;     // Skills tab, current tree
    std::unordered_set<uint32_t> m_heldItems;     // item ids in the inventory (learned skills)
    fssm::parse::sekiro::SekiroCharacterInfo m_stats;   // without items
    bool m_hasCharacter = false;
    int m_page = 0;                 // index into PAGES
    int m_category = 0;             // index into the page's categories
    fssm::ui::PaintedScroll m_scroll {this};    // item grid; range kept by updateScrollRange()
    int m_hoverItem = -1;           // list row, or index into m_treeNodes on the Skills tab
    int m_selectedItem = -1;

    Layout m_layout;
    PixmapCache m_cache;
    WorldProgressWidget* m_worldWidget = nullptr;
};
}
