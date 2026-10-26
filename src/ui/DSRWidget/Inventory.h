#pragma once

#include <QHash>
#include <QPixmap>
#include <QWidget>
#include <vector>

#include "../ScrollHelper.h"
#include "../../parse/DSR/SaveFile.h"

namespace fssm::ui::dsr {
// Inventory screen drawn like the game's list view: a bar of category icons over
// rows of items standing on their dishes (count, red seal on equipped items, burnt
// orange selection), and the selected item's details in a ruled panel like the
// game's item description, beside the list or below it when narrow.
class InventoryWidget: public QWidget {
    Q_OBJECT
public:
    explicit InventoryWidget(QWidget* parent);
    void setCharacter(const fssm::parse::dsr::DSRCharacterInfo* charInfo);

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
        QString name;               // with the upgrade level, "Estus Flask+7"
        QString image;              // :/dsr_inv_images/<image>, empty when unknown
        QString infusionImage;
        QString infusion;           // infusion name, empty for none
        fssm::parse::dsr::ItemCategory category;
        uint32_t amount = 0;        // held
        uint32_t stored = 0;        // in the Bottomless Box
        bool held = false;
        bool inBox = false;
        uint32_t durability = 0;
        uint32_t order = 0;
        uint32_t maxStack = 1;
        bool equipped = false;
        bool known = true;
    };

    struct Layout {
        QRect tabs;
        int tabWidth = 0;
        QRect list;                 // rows, right of the scroll track
        QRect scrollTrack;          // left of the rows, as in the game
        QRect detail;
    };

    Layout computeLayout() const;
    void rebuildVisibleItems();
    void selectCategory(int category);
    void updateScrollRange();
    int itemIndexAt(const QPoint& pos) const;
    int tabIndexAt(const QPoint& pos) const;
    QPixmap scaled(const QString& resource, const QSize& size);
    void drawCategories(QPainter& painter);
    void drawList(QPainter& painter);
    void drawDetail(QPainter& painter);
    void drawItemIcon(QPainter& painter, const Entry& entry, const QRect& rect);

    std::vector<Entry> m_entries;
    std::vector<const Entry*> m_visible;
    bool m_hasCharacter = false;
    int m_category = 0;             // index into CATEGORY_ORDER
    int m_hoverTab = -1;
    int m_hoverItem = -1;
    int m_selectedItem = 0;
    fssm::ui::PaintedScroll m_scroll {this};
    Layout m_layout;
    QHash<QString, QPixmap> m_scaledCache;
};
}
