#include "Inventory.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QWheelEvent>
#include <algorithm>
#include <array>
#include <cmath>

#include "Painting.h"
#include "../Utils.h"

namespace fssm::ui::ds2 {
using fssm::parse::ds2::InventoryItem;
using fssm::parse::ds2::ItemCategory;

namespace {
    // Same order as the in-game inventory category tabs.
    constexpr std::array<ItemCategory, 15> CATEGORY_ORDER = {
        ItemCategory::Consumables,
        ItemCategory::Weapons,
        ItemCategory::RangedWeapons,
        ItemCategory::Catalysts,
        ItemCategory::Shields,
        ItemCategory::HeadArmor,
        ItemCategory::ChestArmor,
        ItemCategory::HandsArmor,
        ItemCategory::LegsArmor,
        ItemCategory::Rings,
        ItemCategory::Ammunition,
        ItemCategory::Spells,
        ItemCategory::KeyItems,
        ItemCategory::Materials,
        ItemCategory::Gestures,
    };

    // ui_window.png is 880x543; borders that must not be stretched.
    constexpr int WINDOW_W = 880;
    constexpr int WINDOW_H = 543;
    constexpr int FRAME_LEFT = 44;
    constexpr int FRAME_TOP = 66;
    constexpr int FRAME_RIGHT = 44;
    constexpr int FRAME_BOTTOM = 44;
    // Item icons are 128x256 with the art roughly in this band.
    const QRect ICON_SOURCE(0, 40, 128, 176);

    constexpr uint32_t ESTUS_FLASK_ID = 60155000;

    // Items you can hold only one of draw no number. The Estus Flask is one
    // item whose amount is its charges, shown like a stack while it has any.
    // Spells hold their casts as the amount, which the game doesn't show here.
    bool showsCount(const InventoryItem& item) {
        if (item.itemId == ESTUS_FLASK_ID) return item.amount > 0;
        if (item.baseItem.category == ItemCategory::Spells) return false;
        return item.baseItem.maxHeld > 1 || item.amount > 1;
    }

    QString categoryLabel(ItemCategory category) {
        switch (category) {
            case ItemCategory::Consumables: return "Consumables";
            case ItemCategory::Weapons: return "Weapons";
            case ItemCategory::RangedWeapons: return "Bows & Crossbows";
            case ItemCategory::Catalysts: return "Catalysts";
            case ItemCategory::Shields: return "Shields";
            case ItemCategory::HeadArmor: return "Head";
            case ItemCategory::ChestArmor: return "Chest";
            case ItemCategory::HandsArmor: return "Hands";
            case ItemCategory::LegsArmor: return "Legs";
            case ItemCategory::Rings: return "Rings";
            case ItemCategory::Ammunition: return "Arrows & Bolts";
            case ItemCategory::Spells: return "Spells";
            case ItemCategory::KeyItems: return "Key Items";
            case ItemCategory::Materials: return "Upgrade Materials";
            case ItemCategory::Gestures: return "Gestures";
            case ItemCategory::Unknown: break;
        }
        return "Unknown";
    }

}

InventoryWidget::InventoryWidget(QWidget* parent): QWidget(parent) {
    setAttribute(Qt::WA_TranslucentBackground, true);
    setMouseTracking(true);
    m_window = QPixmap(":/ds2_images/ui_window");
}

QSize InventoryWidget::sizeHint() const {
    return {WINDOW_W, WINDOW_H};
}

void InventoryWidget::setCharacter(const fssm::parse::ds2::DS2CharacterInfo* charInfo) {
    m_hasCharacter = charInfo != nullptr;
    m_allItems = charInfo != nullptr ? charInfo->inventoryItems : std::vector<InventoryItem>{};
    std::stable_sort(m_allItems.begin(), m_allItems.end(), [](const InventoryItem& a, const InventoryItem& b) {
        if (a.baseItem.order != b.baseItem.order) return a.baseItem.order < b.baseItem.order;
        return a.itemId < b.itemId;
    });
    m_selectedItem = -1;
    m_hoverItem = -1;
    m_scroll = 0;
    rebuildVisibleItems();
    update();
}

void InventoryWidget::rebuildVisibleItems() {
    m_visibleItems.clear();
    const ItemCategory category = CATEGORY_ORDER[m_category];
    for (const auto& item : m_allItems) {
        if (item.baseItem.category == category) m_visibleItems.push_back(&item);
    }
    m_scroll = std::clamp(m_scroll, 0, maxScroll());
}

InventoryWidget::Layout InventoryWidget::computeLayout() const {
    Layout l;
    const QRect r = rect();
    l.scale = FIXED_SCALE;
    const double s = l.scale;
    l.titleBar = QRect(r.left() + int(46 * s), r.top() + int(24 * s), r.width() - int(92 * s), int(36 * s));
    l.content = r.adjusted(int(FRAME_LEFT * s) + 4, int(FRAME_TOP * s) + 6,
                           -(int(FRAME_RIGHT * s) + 4), -(int(FRAME_BOTTOM * s) + 2));

    const int arrowW = std::max(10, int(16 * s));
    const int catArea = l.content.width() - 2 * arrowW;
    // Category tiles keep one size; a wider window shows more of them.
    l.categorySize = CATEGORY_SIZE;
    l.visibleCategories = std::clamp(catArea / CATEGORY_SIZE, 1, int(CATEGORY_ORDER.size()));
    const int catRowWidth = l.categorySize * l.visibleCategories;
    const int catLeft = l.content.left() + (l.content.width() - catRowWidth) / 2;
    l.categories = QRect(catLeft, l.content.top(), catRowWidth, l.categorySize);
    l.leftArrow = QRect(catLeft - arrowW, l.content.top(), arrowW, l.categorySize);
    l.rightArrow = QRect(l.categories.right() + 1, l.content.top(), arrowW, l.categorySize);

    const int nameBarH = std::max(20, int(34 * s));
    l.nameBar = QRect(l.content.left(), l.content.bottom() - nameBarH, l.content.width(), nameBarH);

    const int gridTop = l.categories.bottom() + 1 + std::max(6, int(12 * s));
    const int gridBottom = l.nameBar.top() - std::max(4, int(8 * s));
    // Tiles keep one size; resizing the window adds or removes columns.
    l.tileSize = TILE_SIZE;
    l.rowHeight = int(TILE_SIZE * 1.1);
    l.columns = std::max(1, l.content.width() / TILE_SIZE);
    const int gridWidth = l.tileSize * l.columns;
    l.grid = QRect(l.content.left() + (l.content.width() - gridWidth) / 2, gridTop, gridWidth,
                   std::max(0, gridBottom - gridTop));
    return l;
}

int InventoryWidget::maxScroll() const {
    if (m_layout.rowHeight <= 0) return 0;
    const int columns = std::max(1, m_layout.columns);
    const int rows = int((m_visibleItems.size() + columns - 1) / columns);
    return std::max(0, rows * m_layout.rowHeight - m_layout.grid.height());
}

int InventoryWidget::maxCategoryOffset() const {
    return std::max(0, int(CATEGORY_ORDER.size()) - m_layout.visibleCategories);
}

void InventoryWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    m_layout = computeLayout();
    m_categoryOffset = std::clamp(m_categoryOffset, 0, maxCategoryOffset());
    m_scaledCache.clear();
    m_scroll = std::clamp(m_scroll, 0, maxScroll());
}

QPixmap InventoryWidget::scaled(const QString& resource, const QSize& size) {
    const QString key = resource + QString("@%1x%2").arg(size.width()).arg(size.height());
    auto it = m_scaledCache.find(key);
    if (it != m_scaledCache.end()) return *it;
    QPixmap source(resource);
    QPixmap result;
    if (!source.isNull()) {
        if (resource.startsWith(":/ds2_inv_images/")) source = source.copy(ICON_SOURCE);
        result = source.scaled(size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }
    m_scaledCache.insert(key, result);
    return result;
}

void InventoryWidget::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    painter.setClipRect(event->rect());
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter.setRenderHint(QPainter::Antialiasing, true);
    if (m_layout.tileSize == 0) m_layout = computeLayout();

    drawFrame(painter);
    if (!m_hasCharacter) {
        painter.setFont(serifFont(int(20 * m_layout.scale)));
        painter.setPen(DIM_TEXT_COLOR);
        painter.drawText(m_layout.content, Qt::AlignCenter, "No character selected");
        return;
    }
    drawCategories(painter);
    drawGrid(painter);
    drawNameBar(painter);
}

void InventoryWidget::drawFrame(QPainter& painter) {
    if (!m_window.isNull()) {
        drawNineSlice(painter, m_window, rect(), FRAME_LEFT, FRAME_TOP, FRAME_RIGHT, FRAME_BOTTOM, m_layout.scale);
    } else {
        painter.fillRect(rect(), QColor(32, 30, 28));
    }
    const QRect title = m_layout.titleBar.adjusted(int(14 * m_layout.scale), 0, 0, 0);
    painter.setFont(serifFont(std::max(15, int(m_layout.titleBar.height() * 0.68))));
    drawOutlinedText(painter, title, Qt::AlignLeft | Qt::AlignVCenter, "Inventory", TEXT_COLOR);
}

void InventoryWidget::drawCategories(QPainter& painter) {
    const Layout& l = m_layout;
    const int size = l.categorySize;
    const QPixmap tileBg = scaled(":/ds2_images/ui_category_tile", QSize(size - 4, size - 4));
    const QPixmap selectedBg = scaled(":/ds2_images/ui_category_selected", QSize(size - 2, size - 2));
    for (int i = 0; i < l.visibleCategories; ++i) {
        const int categoryIdx = m_categoryOffset + i;
        if (categoryIdx >= int(CATEGORY_ORDER.size())) break;
        const QRect cell(l.categories.left() + i * size, l.categories.top(), size, size);
        const bool selected = categoryIdx == m_category;
        const bool hovered = !selected && categoryIdx == m_hoverCategory;
        painter.fillRect(cell.adjusted(1, 1, -1, -1), QColor(14, 13, 12, 200));
        if (selected) {
            painter.drawPixmap(cell.topLeft() + QPoint(1, 1), selectedBg);
        } else {
            painter.drawPixmap(cell.topLeft() + QPoint(2, 2), tileBg);
            if (hovered) {
                // A faint version of the selection highlight, like the game's cursor preview.
                painter.setOpacity(0.45);
                painter.drawPixmap(cell.topLeft() + QPoint(1, 1), selectedBg);
                painter.setOpacity(1.0);
            }
        }
        const QString iconName = QString::fromUtf8(
            fssm::parse::ds2::getCategoryIcon(CATEGORY_ORDER[categoryIdx]).data());
        const QPixmap icon = scaled(":/ds2_images/" + iconName, QSize(size - 8, size - 8));
        painter.drawPixmap(cell.topLeft() + QPoint(4, 4), icon);
        painter.setPen(QPen(hovered ? QColor(214, 170, 96) : QColor(80, 72, 58), 1));
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(cell.adjusted(0, 0, -1, -1));
    }

    // Scroll arrows, drawn like the game's golden chevrons.
    auto drawArrow = [&](const QRect& r, bool left, bool enabled) {
        if (!enabled) return;
        const QPointF c = QRectF(r).center();
        const double w = r.width() * 0.35;
        const double h = r.height() * 0.18;
        QPainterPath path;
        if (left) {
            path.moveTo(c.x() + w / 2, c.y() - h);
            path.lineTo(c.x() - w / 2, c.y());
            path.lineTo(c.x() + w / 2, c.y() + h);
        } else {
            path.moveTo(c.x() - w / 2, c.y() - h);
            path.lineTo(c.x() + w / 2, c.y());
            path.lineTo(c.x() - w / 2, c.y() + h);
        }
        path.closeSubpath();
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(232, 172, 40));
        painter.drawPath(path);
    };
    drawArrow(l.leftArrow, true, m_categoryOffset > 0);
    drawArrow(l.rightArrow, false, m_categoryOffset + l.visibleCategories < int(CATEGORY_ORDER.size()));
}

void InventoryWidget::drawGrid(QPainter& painter) {
    const Layout& l = m_layout;
    painter.save();
    painter.setClipRect(l.grid.adjusted(0, 0, 0, 0), Qt::IntersectClip);

    const int tile = l.tileSize;
    const int rowH = l.rowHeight;
    const QSize tileSize(tile - 4, rowH - 4);
    const QPixmap tileBg = scaled(":/ds2_images/ui_item_tile", tileSize);
    const QPixmap selectedBg = scaled(":/ds2_images/ui_item_selected", tileSize);
    const int pedestalW = int(tile * 0.78);
    const QPixmap pedestal = scaled(":/ds2_images/ui_pedestal", QSize(pedestalW, pedestalW * 30 / 70));
    // Icons stand on the pedestal and must stay inside the tile frame: their height is
    // the space between the frame's top edge (plus a small margin) and the pedestal.
    const int innerH = rowH - 4;
    const int pedestalTop = innerH - pedestal.height() - int(tile * 0.04);
    const int iconBottomOffset = pedestalTop + pedestal.height() / 2;
    const int iconTopMargin = std::max(3, int(tile * 0.08));
    const int iconH = std::max(8, iconBottomOffset - iconTopMargin);
    const QSize iconSize(iconH * ICON_SOURCE.width() / ICON_SOURCE.height(), iconH);

    const int firstRow = m_scroll / rowH;
    const int lastRow = (m_scroll + l.grid.height()) / rowH;
    painter.setFont(serifFont(std::max(11, int(tile * 0.24))));
    for (int row = firstRow; row <= lastRow; ++row) {
        for (int col = 0; col < l.columns; ++col) {
            const int idx = row * l.columns + col;
            if (idx >= int(m_visibleItems.size())) break;
            const InventoryItem& item = *m_visibleItems[idx];
            const QRect cell(l.grid.left() + col * tile, l.grid.top() + row * rowH - m_scroll, tile, rowH);
            const QRect inner = cell.adjusted(2, 2, -2, -2);

            const bool highlighted = idx == m_hoverItem || idx == m_selectedItem;
            painter.drawPixmap(inner.topLeft(), highlighted ? selectedBg : tileBg);

            const QPoint pedestalPos(inner.center().x() - pedestal.width() / 2,
                                     inner.bottom() - pedestal.height() - int(tile * 0.04));
            painter.drawPixmap(pedestalPos, pedestal);

            QPixmap icon;
            if (inventoryResourcesAvailable() && !item.baseItem.image.empty()) {
                icon = scaled(":/ds2_inv_images/" + QString::fromUtf8(item.baseItem.image.data(),
                                                                      int(item.baseItem.image.size())), iconSize);
            }
            if (!icon.isNull()) {
                const int iconBottom = inner.top() + iconBottomOffset;
                painter.save();
                painter.setClipRect(inner, Qt::IntersectClip);
                painter.drawPixmap(inner.center().x() - icon.width() / 2, iconBottom - icon.height(), icon);
                painter.restore();
            } else {
                const QString iconName = QString::fromUtf8(
                    fssm::parse::ds2::getCategoryIcon(item.baseItem.category).data());
                const int s = int(tile * 0.6);
                painter.drawPixmap(inner.center().x() - s / 2, inner.top() + int(tile * 0.12),
                                   scaled(":/ds2_images/" + iconName, QSize(s, s)));
            }

            QString label;
            if (item.durability.has_value()) {
                if (item.upgradeLevel > 0) label = QString("+%1").arg(item.upgradeLevel);
            } else if (showsCount(item)) {
                label = QString::number(item.amount);
            }
            if (!label.isEmpty()) {
                drawOutlinedText(painter, inner.adjusted(0, 0, -int(tile * 0.06), -int(tile * 0.02)),
                                 Qt::AlignRight | Qt::AlignBottom, label, TEXT_COLOR);
            }
        }
    }
    painter.restore();

    // Thin scroll indicator on the right edge of the grid.
    const int total = maxScroll();
    if (total > 0) {
        const int trackX = l.grid.right() + std::max(3, int(6 * l.scale));
        const QRect track(trackX, l.grid.top(), std::max(3, int(4 * l.scale)), l.grid.height());
        const int thumbH = std::max(16, track.height() * track.height() / (track.height() + total));
        const int thumbY = track.top() + (track.height() - thumbH) * m_scroll / total;
        painter.fillRect(track, QColor(0, 0, 0, 120));
        painter.fillRect(QRect(track.left(), thumbY, track.width(), thumbH), QColor(196, 150, 60));
    }
}

QString InventoryWidget::itemTitle(const InventoryItem& item) const {
    QString title;
    const auto infusion = fssm::parse::ds2::infusionLabel(item.infusion);
    if (!infusion.empty()) title = QString::fromUtf8(infusion.data(), int(infusion.size())) + " ";
    if (item.knownItem) {
        title += QString::fromUtf8(item.baseItem.label.data(), int(item.baseItem.label.size()));
    } else {
        title += QString("Unknown item %1").arg(item.itemId);
    }
    if (item.upgradeLevel > 0) title += QString(" +%1").arg(item.upgradeLevel);
    return title;
}

void InventoryWidget::drawNameBar(QPainter& painter) {
    const QRect bar = m_layout.nameBar;
    QLinearGradient gradient(bar.topLeft(), bar.bottomLeft());
    gradient.setColorAt(0.0, QColor(42, 58, 66, 230));
    gradient.setColorAt(0.5, QColor(22, 32, 38, 230));
    gradient.setColorAt(1.0, QColor(12, 16, 20, 230));
    painter.setPen(QPen(QColor(112, 100, 76), 1));
    painter.setBrush(gradient);
    painter.drawRect(bar.adjusted(0, 0, -1, -1));

    const int idx = m_hoverItem >= 0 ? m_hoverItem : m_selectedItem;
    const QRect text = bar.adjusted(int(12 * m_layout.scale), 0, -int(12 * m_layout.scale), 0);
    painter.setFont(serifFont(std::max(12, int(bar.height() * 0.55))));
    if (idx < 0 || idx >= int(m_visibleItems.size())) {
        const QString label = QString("%1  (%2)").arg(categoryLabel(CATEGORY_ORDER[m_category]))
                                                  .arg(m_visibleItems.size());
        drawOutlinedText(painter, text, Qt::AlignLeft | Qt::AlignVCenter, label, DIM_TEXT_COLOR);
        return;
    }
    const InventoryItem& item = *m_visibleItems[idx];
    drawOutlinedText(painter, text, Qt::AlignLeft | Qt::AlignVCenter, itemTitle(item), TEXT_COLOR);
    QString detail;
    if (item.durability.has_value()) {
        detail = QString("Durability %1").arg(int(std::round(*item.durability)));
    } else if (showsCount(item)) {
        detail = QString("x %1").arg(item.amount);
    }
    drawOutlinedText(painter, text, Qt::AlignRight | Qt::AlignVCenter, detail, DIM_TEXT_COLOR);
}

int InventoryWidget::itemIndexAt(const QPoint& pos) const {
    const Layout& l = m_layout;
    if (!l.grid.contains(pos) || l.tileSize <= 0) return -1;
    const int col = (pos.x() - l.grid.left()) / l.tileSize;
    const int row = (pos.y() - l.grid.top() + m_scroll) / l.rowHeight;
    if (col < 0 || col >= l.columns) return -1;
    const int idx = row * l.columns + col;
    return idx < int(m_visibleItems.size()) ? idx : -1;
}

int InventoryWidget::categoryIndexAt(const QPoint& pos) const {
    const Layout& l = m_layout;
    if (!l.categories.contains(pos) || l.categorySize <= 0) return -1;
    const int idx = m_categoryOffset + (pos.x() - l.categories.left()) / l.categorySize;
    return idx < int(CATEGORY_ORDER.size()) ? idx : -1;
}

void InventoryWidget::mouseMoveEvent(QMouseEvent* event) {
    const int hover = m_hasCharacter ? itemIndexAt(event->pos()) : -1;
    if (hover != m_hoverItem) {
        m_hoverItem = hover;
        update();
    }
    const int category = m_hasCharacter ? categoryIndexAt(event->pos()) : -1;
    if (category != m_hoverCategory) {
        m_hoverCategory = category;
        update(m_layout.categories.adjusted(-2, -2, 2, 2));
    }
    setToolTip(category >= 0 ? categoryLabel(CATEGORY_ORDER[category]) : QString());
    QWidget::mouseMoveEvent(event);
}

void InventoryWidget::mousePressEvent(QMouseEvent* event) {
    if (!m_hasCharacter || event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }
    const QPoint pos = event->pos();
    const int maxOffset = maxCategoryOffset();
    if (m_layout.leftArrow.contains(pos)) {
        m_categoryOffset = std::max(0, m_categoryOffset - 1);
    } else if (m_layout.rightArrow.contains(pos)) {
        m_categoryOffset = std::min(maxOffset, m_categoryOffset + 1);
    } else if (const int category = categoryIndexAt(pos); category >= 0) {
        if (category != m_category) {
            m_category = category;
            m_scroll = 0;
            m_selectedItem = -1;
            m_hoverItem = -1;
            rebuildVisibleItems();
        }
    } else if (const int item = itemIndexAt(pos); item >= 0) {
        m_selectedItem = item;
    }
    update();
}

void InventoryWidget::wheelEvent(QWheelEvent* event) {
    if (!m_hasCharacter) return;
    const QPoint pos = event->position().toPoint();
    const int steps = event->angleDelta().y() / 120;
    if (m_layout.categories.adjusted(-m_layout.leftArrow.width(), 0, m_layout.rightArrow.width(), 0).contains(pos)) {
        const int maxOffset = maxCategoryOffset();
        m_categoryOffset = std::clamp(m_categoryOffset - steps, 0, maxOffset);
    } else {
        m_scroll = std::clamp(m_scroll - steps * m_layout.rowHeight, 0, maxScroll());
        m_hoverItem = itemIndexAt(pos);
    }
    update();
    event->accept();
}

void InventoryWidget::leaveEvent(QEvent* event) {
    m_hoverItem = -1;
    m_hoverCategory = -1;
    update();
    QWidget::leaveEvent(event);
}
}
