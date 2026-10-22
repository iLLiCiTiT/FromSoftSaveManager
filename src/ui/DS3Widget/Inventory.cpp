#include "Inventory.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QWheelEvent>
#include <algorithm>
#include <array>
#include <cmath>
#include <tuple>
#include <unordered_map>

#include "Painting.h"
#include "../Utils.h"

namespace fssm::ui::ds3 {
using fssm::parse::ds3::Infusion;
using fssm::parse::ds3::InventoryItem;
using fssm::parse::ds3::ItemCategory;

namespace {
    // Same order as the in-game inventory tabs.
    constexpr std::array<ItemCategory, 15> CATEGORY_ORDER = {
        ItemCategory::Tools,
        ItemCategory::Materials,
        ItemCategory::KeyItems,
        ItemCategory::Spells,
        ItemCategory::MeleeWeapons,
        ItemCategory::RangedWeapons,
        ItemCategory::Catalysts,
        ItemCategory::Shields,
        ItemCategory::HeadArmor,
        ItemCategory::ChestArmor,
        ItemCategory::HandsArmor,
        ItemCategory::LegsArmor,
        ItemCategory::ArrowsBolts,
        ItemCategory::Rings,
        ItemCategory::CovenantItem,
    };

    // Reference layout size (the game's inventory at 1080p, scaled down).
    constexpr int DESIGN_W = 560;
    constexpr int DESIGN_H = 470;
    // Tabs, padding, fonts and the detail panel keep one size (the look at the default
    // window size); only the item grid follows the widget size.
    constexpr double FIXED_SCALE = 0.76;
    // Item tile pitch in pixels, independent of the widget size (same as the
    // Elden Ring inventory); resizing the window adds or removes columns.
    constexpr int TILE_SIZE = 60;
    // Items that never show in the game's inventory.
    constexpr uint32_t HIDDEN_ITEM_ID = 1073741918;
    constexpr uint32_t ESTUS_FLASK_ID = 1073741974;
    constexpr uint32_t ASHEN_ESTUS_FLASK_ID = 1073742014;

    QString categoryLabel(ItemCategory category) {
        switch (category) {
            case ItemCategory::Tools: return "Tools";
            case ItemCategory::Materials: return "Materials";
            case ItemCategory::KeyItems: return "Key Items";
            case ItemCategory::Spells: return "Spells";
            case ItemCategory::MeleeWeapons: return "Melee Weapons";
            case ItemCategory::RangedWeapons: return "Ranged Weapons";
            case ItemCategory::Catalysts: return "Catalysts";
            case ItemCategory::Shields: return "Shields";
            case ItemCategory::HeadArmor: return "Head";
            case ItemCategory::ChestArmor: return "Chest";
            case ItemCategory::HandsArmor: return "Hands";
            case ItemCategory::LegsArmor: return "Legs";
            case ItemCategory::ArrowsBolts: return "Arrows/Bolts";
            case ItemCategory::Rings: return "Rings";
            case ItemCategory::CovenantItem: return "Covenants";
        }
        return "Unknown";
    }

    // Item type shown under the name in the detail panel.
    QString typeLabel(ItemCategory category) {
        switch (category) {
            case ItemCategory::Tools: return "Consumable";
            case ItemCategory::Materials: return "Upgrade Material";
            case ItemCategory::KeyItems: return "Key Item";
            case ItemCategory::Spells: return "Spell";
            case ItemCategory::MeleeWeapons: return "Weapon";
            case ItemCategory::RangedWeapons: return "Ranged Weapon";
            case ItemCategory::Catalysts: return "Catalyst";
            case ItemCategory::Shields: return "Shield";
            case ItemCategory::HeadArmor:
            case ItemCategory::ChestArmor:
            case ItemCategory::HandsArmor:
            case ItemCategory::LegsArmor: return "Armor";
            case ItemCategory::ArrowsBolts: return "Ammunition";
            case ItemCategory::Rings: return "Ring";
            case ItemCategory::CovenantItem: return "Covenant";
        }
        return {};
    }

    // {display name, icon resource} of an infusion.
    std::pair<QString, QString> infusionInfo(Infusion infusion) {
        switch (infusion) {
            case Infusion::Heavy: return {"Heavy", "heavy"};
            case Infusion::Sharp: return {"Sharp", "sharp"};
            case Infusion::Refined: return {"Refined", "refined"};
            case Infusion::Simple: return {"Simple", "simple"};
            case Infusion::Crystal: return {"Crystal", "crystal"};
            case Infusion::Fire: return {"Fire", "fire"};
            case Infusion::Chaos: return {"Chaos", "chaos"};
            case Infusion::Lighting: return {"Lightning", "lighting"};
            case Infusion::Deep: return {"Deep", "deep"};
            case Infusion::Dark: return {"Dark", "dark"};
            case Infusion::Poison: return {"Poison", "poison"};
            case Infusion::Blood: return {"Blood", "blood"};
            case Infusion::Raw: return {"Raw", "raw"};
            case Infusion::Blessed: return {"Blessed", "blessed"};
            case Infusion::Hollow: return {"Hollow", "hollow"};
            case Infusion::None: break;
        }
        return {};
    }

    QString fromView(std::string_view text) {
        return QString::fromUtf8(text.data(), int(text.size()));
    }

    QString itemTitle(const InventoryItem& item) {
        if (item.baseItem.id == 0) return QString("Unknown item %1").arg(item.itemId);
        QString title = fromView(item.baseItem.label);
        const auto [infusion, _] = infusionInfo(item.infusion);
        if (!infusion.isEmpty()) {
            const QString pattern = fromView(item.baseItem.infusion_label);
            title = pattern.isEmpty() ? infusion + " " + title : QString(pattern).replace("{infusion}", infusion);
        }
        if (item.upgradeLevel > 0) title += QString("+%1").arg(item.upgradeLevel);
        return title;
    }

    // Estus icons show how full the flask is, relative to its capacity.
    QString flaskImage(const QString& base, uint32_t maxAmount, uint32_t amount) {
        if (amount == 0) return base + "_empty";
        if (maxAmount <= 2) return base;
        static constexpr std::array<std::pair<int, int>, 16> THRESHOLDS = {{
            {0, 0}, {0, 0}, {0, 0}, {2, 1}, {2, 1}, {3, 1}, {4, 2}, {4, 2},
            {5, 3}, {6, 3}, {7, 3}, {7, 4}, {8, 4}, {9, 5}, {9, 5}, {10, 5},
        }};
        const auto [full, half] = THRESHOLDS[std::min<size_t>(maxAmount, THRESHOLDS.size() - 1)];
        if (int(amount) > full) return base;
        if (int(amount) > half) return base + "_half";
        return base + "_quater";
    }
}

InventoryWidget::InventoryWidget(QWidget* parent): QWidget(parent) {
    setAttribute(Qt::WA_TranslucentBackground, true);
    setMouseTracking(true);
    m_tabBackground = QPixmap(":/ds3_images/inv_bg_1");
}

QSize InventoryWidget::sizeHint() const {
    return {DESIGN_W, DESIGN_H};
}

void InventoryWidget::setCharacter(const fssm::parse::ds3::DS3CharacterInfo* charInfo) {
    m_entries.clear();
    m_hasCharacter = charInfo != nullptr;
    if (charInfo != nullptr) {
        std::unordered_map<uint32_t, uint32_t> stored;
        for (const auto& item : charInfo->storageBoxItems) stored[item.itemId] += item.amount;

        const auto addItems = [&](const std::vector<InventoryItem>& items) {
            for (const auto& item : items) {
                if (item.itemId == HIDDEN_ITEM_ID) continue;
                Entry entry;
                entry.item = item;
                entry.title = itemTitle(item);
                entry.image = fromView(item.baseItem.image);
                if (auto it = stored.find(item.itemId); it != stored.end()) entry.stored = it->second;
                if (item.itemId == ESTUS_FLASK_ID) {
                    entry.maxHeld = charInfo->estusMax;
                    entry.image = flaskImage("estus_flask", charInfo->estusMax, item.amount);
                } else if (item.itemId == ASHEN_ESTUS_FLASK_ID) {
                    entry.maxHeld = charInfo->ashenEstusMax;
                    entry.image = flaskImage("ashen_estus_flask", charInfo->ashenEstusMax, item.amount);
                } else if (item.baseItem.max_inventory > 1) {
                    entry.maxHeld = item.baseItem.max_inventory;
                }
                m_entries.push_back(std::move(entry));
            }
        };
        addItems(charInfo->inventoryItems);
        addItems(charInfo->keyItems);
        std::stable_sort(m_entries.begin(), m_entries.end(), [](const Entry& a, const Entry& b) {
            if (a.item.baseItem.order != b.item.baseItem.order) return a.item.baseItem.order < b.item.baseItem.order;
            if (a.item.itemId != b.item.itemId) return a.item.itemId < b.item.itemId;
            return a.item.upgradeLevel > b.item.upgradeLevel;
        });
    }
    m_selectedItem = 0;
    m_hoverItem = -1;
    m_scroll.reset();
    rebuildVisibleItems();
    update();
}

void InventoryWidget::rebuildVisibleItems() {
    m_visible.clear();
    const ItemCategory category = CATEGORY_ORDER[m_category];
    for (const auto& entry : m_entries) {
        if (entry.item.baseItem.category == category) m_visible.push_back(&entry);
    }
    m_selectedItem = m_visible.empty() ? -1 : std::clamp(m_selectedItem, 0, int(m_visible.size()) - 1);
    updateScrollRange();
}

void InventoryWidget::keepCategoryVisible() {
    const int visible = std::max(1, m_layout.visibleTabs);
    if (m_category < m_tabOffset) m_tabOffset = m_category;
    if (m_category >= m_tabOffset + visible) m_tabOffset = m_category - visible + 1;
    m_tabOffset = std::clamp(m_tabOffset, 0, std::max(0, int(CATEGORY_ORDER.size()) - visible));
}

void InventoryWidget::selectCategory(int category) {
    category = std::clamp(category, 0, int(CATEGORY_ORDER.size()) - 1);
    if (category == m_category) return;
    m_category = category;
    keepCategoryVisible();
    m_scroll.reset();
    m_selectedItem = 0;
    m_hoverItem = -1;
    rebuildVisibleItems();
    update();
}

InventoryWidget::Layout InventoryWidget::computeLayout() const {
    Layout l;
    const QRect r = rect();
    l.scale = FIXED_SCALE;
    const double s = l.scale;
    const auto px = [s](double v) { return int(std::round(v * s)); };

    l.panel = r;
    const QRect inner = r.adjusted(px(14), px(8), -px(14), -px(12));

    // Category tabs (the name shows as a tooltip); a wider panel shows more of them.
    const int tabsTop = inner.top() + px(6);
    const int tabHeight = px(56);
    const int arrowW = px(26);
    const int tabsWidth = inner.width() - 2 * arrowW;
    const int maxTabWidth = int(tabHeight * 0.85);
    l.visibleTabs = std::clamp(tabsWidth / maxTabWidth, 5, int(CATEGORY_ORDER.size()));
    l.tabWidth = std::min(tabsWidth / l.visibleTabs, maxTabWidth);
    const int tabsLeft = inner.left() + (inner.width() - l.tabWidth * l.visibleTabs) / 2;
    l.tabs = QRect(tabsLeft, tabsTop, l.tabWidth * l.visibleTabs, tabHeight);
    l.leftArrow = QRect(tabsLeft - arrowW, tabsTop, arrowW, tabHeight);
    l.rightArrow = QRect(l.tabs.right() + 1, tabsTop, arrowW, tabHeight);

    // Selected item details along the bottom.
    const int detailHeight = px(92);
    l.detail = QRect(inner.left(), inner.bottom() - detailHeight + 1, inner.width(), detailHeight);

    const int frameTop = l.tabs.bottom() + 1 + px(8);
    l.gridFrame = QRect(inner.left(), frameTop, inner.width(), l.detail.top() - px(8) - frameTop);
    const QRect frameInner = l.gridFrame.adjusted(px(12), px(10), -px(16), -px(10));
    // Tiles keep one size; resizing the window adds or removes columns.
    l.cellWidth = TILE_SIZE;
    l.columns = std::max(1, frameInner.width() / TILE_SIZE);
    // The game's item cells are about 100x110 px: slightly taller than wide.
    l.rowHeight = TILE_SIZE * 11 / 10;
    l.grid = QRect(frameInner.left() + (frameInner.width() - l.cellWidth * l.columns) / 2, frameInner.top(),
                   l.cellWidth * l.columns, std::max(0, frameInner.height()));
    // In the frame's right margin, so it doesn't move when columns come and go.
    const int barW = std::max(2, px(3));
    l.scrollTrack = QRect(frameInner.right() + px(7), frameInner.top(), barW, l.grid.height());
    return l;
}

void InventoryWidget::updateScrollRange() {
    const int columns = std::max(1, m_layout.columns);
    const int rows = int((m_visible.size() + columns - 1) / columns);
    m_scroll.setGeometry(m_layout.grid, m_layout.scrollTrack, rows * m_layout.rowHeight);
    m_scroll.setStep(m_layout.rowHeight);
}

void InventoryWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    m_layout = computeLayout();
    keepCategoryVisible();
    m_scaledCache.clear();
    updateScrollRange();
}

QPixmap InventoryWidget::scaled(const QString& resource, const QSize& size) {
    const QString key = resource + QString("@%1x%2").arg(size.width()).arg(size.height());
    auto it = m_scaledCache.find(key);
    if (it != m_scaledCache.end()) return *it;
    QPixmap source(resource);
    QPixmap result;
    if (!source.isNull() && !size.isEmpty()) {
        result = source.scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    m_scaledCache.insert(key, result);
    return result;
}

QPixmap InventoryWidget::itemIcon(const Entry& entry, const QSize& size) {
    QPixmap icon;
    if (inventoryResourcesAvailable() && !entry.image.isEmpty()) {
        icon = scaled(":/ds3_inv_images/" + entry.image, size);
    }
    if (icon.isNull()) {
        // Without the icon bundle, fall back to the category's tab icon.
        const QString categoryIcon = fromView(fssm::parse::ds3::getCategoryIcon(entry.item.baseItem.category));
        icon = scaled(":/ds3_images/" + categoryIcon, size * 0.7);
    }
    return icon;
}

const InventoryWidget::Entry* InventoryWidget::currentEntry() const {
    const int idx = m_hoverItem >= 0 ? m_hoverItem : m_selectedItem;
    if (idx < 0 || idx >= int(m_visible.size())) return nullptr;
    return m_visible[idx];
}

void InventoryWidget::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    painter.setClipRect(event->rect());
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);
    if (m_layout.cellWidth == 0) m_layout = computeLayout();

    drawMenuPanel(painter, m_layout.panel, ":/ds3_images/panel_inventory");
    if (!m_hasCharacter) {
        painter.setFont(serifFont(int(20 * m_layout.scale)));
        drawShadowText(painter, m_layout.panel, Qt::AlignCenter, "No character selected", DIM_TEXT_COLOR);
        return;
    }
    drawCategories(painter);
    drawGrid(painter);
    drawDetail(painter);
}

void InventoryWidget::drawCategories(QPainter& painter) {
    const Layout& l = m_layout;
    const double s = l.scale;
    const int w = l.tabWidth;
    for (int i = 0; i < l.visibleTabs; ++i) {
        const int categoryIdx = m_tabOffset + i;
        if (categoryIdx >= int(CATEGORY_ORDER.size())) break;
        const bool selected = categoryIdx == m_category;
        const bool hovered = !selected && categoryIdx == m_hoverTab;
        QRect cell(l.tabs.left() + i * w, l.tabs.top(), w, l.tabs.height());
        cell = selected ? cell.adjusted(1, -int(5 * s), -1, int(3 * s)) : cell.adjusted(int(3 * s), int(3 * s), -int(3 * s), -int(2 * s));

        painter.setOpacity(selected ? 1.0 : (hovered ? 0.75 : 0.4));
        painter.drawPixmap(cell, m_tabBackground);
        painter.setOpacity(selected || hovered ? 1.0 : 0.7);
        const QString iconName = fromView(fssm::parse::ds3::getCategoryIcon(CATEGORY_ORDER[categoryIdx]));
        const QPixmap icon = scaled(":/ds3_images/" + iconName, cell.size() * (selected ? 1.05 : 1.0));
        painter.drawPixmap(cell.center().x() - icon.width() / 2 + 1, cell.center().y() - icon.height() / 2 + 1, icon);
        painter.setOpacity(1.0);
        if (selected || hovered) {
            painter.setPen(QPen(selected ? QColor(210, 200, 180, 160) : QColor(214, 170, 96, 150), 1));
            painter.setBrush(Qt::NoBrush);
            painter.drawRect(cell.adjusted(0, 0, -1, -1));
        }
    }

    // «LB  RB» hints on both sides of the tab strip.
    painter.setFont(serifFont(int(22 * s)));
    drawShadowText(painter, l.leftArrow, Qt::AlignCenter, QString::fromUtf8("«"),
                   m_category > 0 ? TEXT_COLOR : DIM_TEXT_COLOR);
    drawShadowText(painter, l.rightArrow, Qt::AlignCenter, QString::fromUtf8("»"),
                   m_category + 1 < int(CATEGORY_ORDER.size()) ? TEXT_COLOR : DIM_TEXT_COLOR);
}

void InventoryWidget::drawGrid(QPainter& painter) {
    const Layout& l = m_layout;
    const double s = l.scale;

    drawOrnateFrame(painter, l.gridFrame, s);

    painter.save();
    painter.setClipRect(l.grid, Qt::IntersectClip);
    const int cw = l.cellWidth;
    const int rh = l.rowHeight;
    const int dishW = int(cw * 0.74);
    const QPixmap dish = scaled(":/ds3_images/dish", QSize(dishW, dishW));
    const int iconSize = int(cw * 0.8);
    const int firstRow = m_scroll.value() / rh;
    const int lastRow = (m_scroll.value() + l.grid.height()) / rh;
    // The game's cell layers (tools/ds3_flags/gen_menu.py) on every cell, filled or not:
    // a shadow at the top (multiply) and a highlight at the bottom (screen), then grid
    // lines from a cross centred on each cell corner (multiply). Opacities are the .gfx's.
    static const QPixmap shadowPix(":/ds3_images/inv_cell_shadow");
    static const QPixmap highlightPix(":/ds3_images/inv_cell_highlight");
    static const QPixmap gridPix(":/ds3_images/inv_cell_grid");
    const auto cellAt = [&](int row, int col) {
        return QRect(l.grid.left() + col * cw, l.grid.top() + row * rh - m_scroll.value(), cw, rh);
    };
    painter.save();
    for (const auto& [pix, mode, opacity] : {
             std::tuple{&shadowPix, QPainter::CompositionMode_Multiply, 115 / 256.0},
             std::tuple{&highlightPix, QPainter::CompositionMode_Screen, 15 / 256.0}}) {
        painter.setCompositionMode(mode);
        painter.setOpacity(opacity);
        for (int row = firstRow; row <= lastRow; ++row) {
            for (int col = 0; col < l.columns; ++col) {
                // The masks are 96 x 106 in the game's 100 x 110 cells.
                const QRect cell = cellAt(row, col);
                painter.drawPixmap(cell.adjusted(cw * 2 / 100, rh * 2 / 110, -cw * 2 / 100, -rh * 2 / 110), *pix);
            }
        }
    }
    painter.setCompositionMode(QPainter::CompositionMode_Multiply);
    painter.setOpacity(26 / 256.0);
    for (int row = firstRow; row <= lastRow + 1; ++row) {
        for (int col = 0; col <= l.columns; ++col) {
            painter.drawPixmap(cellAt(row, col).translated(-cw / 2, -rh / 2), gridPix);
        }
    }
    painter.restore();

    painter.setFont(serifFont(std::max(11, int(cw * 0.2))));
    for (int row = firstRow; row <= lastRow; ++row) {
        for (int col = 0; col < l.columns; ++col) {
            const int idx = row * l.columns + col;
            if (idx >= int(m_visible.size())) break;
            const Entry& entry = *m_visible[idx];
            const QRect cell(l.grid.left() + col * cw, l.grid.top() + row * rh - m_scroll.value(), cw, rh);
            const int pad = std::max(1, int(cw * 0.03));
            const QRect tile = cell.adjusted(pad, pad, -pad, -pad);

            // The selected cell gets the game's orange square (fainter on hover).
            static const QPixmap cursorPix(":/ds3_images/inv_cell_cursor");
            if (idx == m_selectedItem || idx == m_hoverItem) {
                painter.setOpacity(idx == m_selectedItem ? 1.0 : 0.4);
                painter.drawPixmap(tile, cursorPix);
                painter.setOpacity(1.0);
            }

            const QPoint dishPos(tile.center().x() - dish.width() / 2, tile.bottom() - dish.height() - int(cw * 0.04));
            painter.drawPixmap(dishPos, dish);
            const QPixmap icon = itemIcon(entry, QSize(iconSize, iconSize));
            const int iconBottom = dishPos.y() + dish.height() * 2 / 3;
            painter.drawPixmap(tile.center().x() - icon.width() / 2, iconBottom - icon.height(), icon);

            if (entry.item.equipped || entry.item.inQuickMenu) {
                // The game's wax seal: red when equipped, grey when only in the menu's quick items.
                const int sealSize = int(cw * 0.3);
                const QString seal = entry.item.equipped ? ":/ds3_images/seal_equipped" : ":/ds3_images/seal_quick_menu";
                painter.drawPixmap(tile.left() + int(cw * 0.03), tile.top() + int(cw * 0.03),
                                   scaled(seal, QSize(sealSize, sealSize)));
            }
            const auto [infusion, infusionIcon] = infusionInfo(entry.item.infusion);
            if (!infusionIcon.isEmpty() && inventoryResourcesAvailable()) {
                const int sz = int(cw * 0.24);
                painter.drawPixmap(tile.right() - sz - int(cw * 0.04), tile.top() + int(cw * 0.04),
                                   scaled(":/ds3_inv_images/" + infusionIcon, QSize(sz, sz)));
            }

            QString label;
            if (entry.maxHeld > 0 || entry.item.baseItem.category == ItemCategory::ArrowsBolts) {
                label = QString::number(entry.item.amount);
            } else if (entry.item.upgradeLevel > 0) {
                label = QString("+%1").arg(entry.item.upgradeLevel);
            }
            if (!label.isEmpty()) {
                drawShadowText(painter, tile.adjusted(0, 0, -int(cw * 0.06), -int(cw * 0.02)),
                               Qt::AlignRight | Qt::AlignBottom, label);
            }
        }
    }
    painter.restore();

    // Thin scroll bar along the right edge of the grid frame.
    m_scroll.paint(painter, QColor(0, 0, 0, 120), QColor(186, 150, 80));
}

void InventoryWidget::drawDetail(QPainter& painter) {
    const QRect area = m_layout.detail;
    const double s = m_layout.scale;
    const auto px = [s](double v) { return int(std::round(v * s)); };
    painter.save();
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(16, 15, 15, 150));
    painter.drawRect(area);
    painter.restore();
    drawGoldRule(painter, area.left(), area.right(), area.top(), 220);
    const Entry* entry = currentEntry();
    if (entry == nullptr) return;

    // Icon on its dish at the left.
    const int iconSize = area.height() - px(6);
    const QRect iconArea(area.left() + px(6), area.top() + px(3), iconSize, iconSize);
    const QPixmap dish = scaled(":/ds3_images/dish", QSize(int(iconSize * 0.8), iconSize));
    const QPoint dishPos(iconArea.center().x() - dish.width() / 2, iconArea.bottom() - dish.height());
    painter.drawPixmap(dishPos, dish);
    const QPixmap icon = itemIcon(*entry, QSize(iconSize, iconSize));
    painter.drawPixmap(iconArea.center().x() - icon.width() / 2,
                       dishPos.y() + dish.height() * 2 / 3 - icon.height(), icon);

    // Text columns: name/type/equipped, held/stored, reinforcement/infusion. When narrow,
    // the last two share one column and gear shows reinforcement/infusion there.
    const QRect text(iconArea.right() + px(12), area.top() + px(6), area.right() - iconArea.right() - px(18),
                     area.height() - px(12));
    const bool wide = text.width() >= px(560);
    const int lineH = text.height() / 3;
    const int col1W = text.width() * (wide ? 40 : 50) / 100;
    const int col2W = wide ? text.width() * 30 / 100 : text.width() - col1W;
    const int gap = px(14);
    const QRect col1(text.left(), text.top(), col1W - gap, text.height());
    const QRect col2(text.left() + col1W, text.top(), col2W - (wide ? gap : 0), text.height());
    const QRect col3(text.left() + col1W + col2W, text.top(), text.width() - col1W - col2W, text.height());
    const auto line = [&](const QRect& col, int row) {
        return QRect(col.left(), col.top() + row * lineH, col.width(), lineH);
    };
    const int smallIcon = std::min(px(18), lineH - 2);
    const auto valueRow = [&](const QRect& col, int row, const QPixmap& rowIcon, const QString& label,
                              const QString& value) {
        const QRect r = line(col, row);
        int textLeft = r.left();
        if (!rowIcon.isNull()) {
            painter.drawPixmap(r.left(), r.center().y() - rowIcon.height() / 2, rowIcon);
            textLeft += rowIcon.width() + px(4);
        }
        drawShadowText(painter, QRect(textLeft, r.top(), r.right() - textLeft, r.height()),
                       Qt::AlignLeft | Qt::AlignVCenter, label);
        if (!value.isEmpty()) drawShadowText(painter, r, Qt::AlignRight | Qt::AlignVCenter, value);
        drawGoldRule(painter, r.left(), r.right(), r.bottom(), 120);
    };

    const ItemCategory category = entry->item.baseItem.category;
    painter.setFont(serifFont(px(19)));
    drawShadowText(painter, line(col1, 0), Qt::AlignLeft | Qt::AlignVCenter, entry->title);
    painter.setFont(serifFont(px(15)));
    const QString categoryIcon = fromView(fssm::parse::ds3::getCategoryIcon(category));
    valueRow(col1, 1, scaled(":/ds3_images/" + categoryIcon, QSize(smallIcon, smallIcon)), typeLabel(category), {});
    if (entry->item.equipped || entry->item.inQuickMenu) {
        const QRect r = line(col1, 2);
        const QString seal = entry->item.equipped ? ":/ds3_images/seal_equipped" : ":/ds3_images/seal_quick_menu";
        painter.drawPixmap(r.left(), r.center().y() - smallIcon / 2, scaled(seal, QSize(smallIcon, smallIcon)));
        drawShadowText(painter, r.adjusted(smallIcon + px(4), 0, 0, 0), Qt::AlignLeft | Qt::AlignVCenter,
                       entry->item.equipped ? "Equipped" : "Quick menu");
    }

    const auto [infusion, infusionIcon] = infusionInfo(entry->item.infusion);
    const bool isEquipment = entry->maxHeld == 0 && category != ItemCategory::KeyItems
        && category != ItemCategory::Spells && category != ItemCategory::CovenantItem;
    const auto drawGearRows = [&](const QRect& col) {
        int row = 0;
        if (isEquipment) {
            valueRow(col, row++, {}, "Reinforcement",
                     entry->item.upgradeLevel > 0 ? QString("+%1").arg(entry->item.upgradeLevel) : "-");
        }
        if (!infusion.isEmpty()) valueRow(col, row++, {}, "Infusion", infusion);
    };
    if (!wide && (isEquipment || !infusion.isEmpty())) {
        drawGearRows(col2);
        return;
    }

    const QPixmap bagIcon = scaled(":/ds3_images/menu_invetory", QSize(smallIcon, smallIcon));
    const QPixmap boxIcon = scaled(":/ds3_images/menu_storage_box", QSize(smallIcon, smallIcon));
    if (entry->maxHeld > 0) {
        valueRow(col2, 0, bagIcon, "No. Held", QString("%1/ %2").arg(entry->item.amount).arg(entry->maxHeld));
        const uint16_t maxStored = entry->item.baseItem.max_storage;
        valueRow(col2, 1, boxIcon, "Stored",
                 maxStored > 0 ? QString("%1/ %2").arg(entry->stored).arg(maxStored) : "-/  -");
    } else {
        valueRow(col2, 0, bagIcon, "No. Held", QString::number(entry->item.amount));
        // Gear in the box can't be told apart from the held piece by id alone.
        valueRow(col2, 1, boxIcon, "Stored", "-");
    }
    if (wide) drawGearRows(col3);
}

int InventoryWidget::itemIndexAt(const QPoint& pos) const {
    const Layout& l = m_layout;
    if (!l.grid.contains(pos) || l.cellWidth <= 0) return -1;
    const int col = (pos.x() - l.grid.left()) / l.cellWidth;
    const int row = (pos.y() - l.grid.top() + m_scroll.value()) / l.rowHeight;
    if (col < 0 || col >= l.columns) return -1;
    const int idx = row * l.columns + col;
    return idx < int(m_visible.size()) ? idx : -1;
}

int InventoryWidget::tabIndexAt(const QPoint& pos) const {
    const Layout& l = m_layout;
    if (!l.tabs.contains(pos) || l.tabWidth <= 0) return -1;
    const int idx = m_tabOffset + (pos.x() - l.tabs.left()) / l.tabWidth;
    return idx < int(CATEGORY_ORDER.size()) ? idx : -1;
}

void InventoryWidget::mouseMoveEvent(QMouseEvent* event) {
    if (m_scroll.mouseMove(event)) return;
    const int hover = m_hasCharacter ? itemIndexAt(event->pos()) : -1;
    if (hover != m_hoverItem) {
        m_hoverItem = hover;
        update();
    }
    const int tab = m_hasCharacter ? tabIndexAt(event->pos()) : -1;
    if (tab != m_hoverTab) {
        m_hoverTab = tab;
        update();
    }
    setToolTip(tab >= 0 ? categoryLabel(CATEGORY_ORDER[tab]) : QString());
    QWidget::mouseMoveEvent(event);
}

void InventoryWidget::mousePressEvent(QMouseEvent* event) {
    if (m_hasCharacter && m_scroll.mousePress(event)) return;
    if (!m_hasCharacter || event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }
    const QPoint pos = event->pos();
    if (m_layout.leftArrow.contains(pos)) {
        selectCategory(m_category - 1);
    } else if (m_layout.rightArrow.contains(pos)) {
        selectCategory(m_category + 1);
    } else if (const int tab = tabIndexAt(pos); tab >= 0) {
        selectCategory(tab);
    } else if (const int item = itemIndexAt(pos); item >= 0) {
        m_selectedItem = item;
        update();
    }
}

void InventoryWidget::mouseReleaseEvent(QMouseEvent* event) {
    if (!m_scroll.mouseRelease(event)) QWidget::mouseReleaseEvent(event);
}

void InventoryWidget::wheelEvent(QWheelEvent* event) {
    if (!m_hasCharacter) return;
    const QPoint pos = event->position().toPoint();
    const int steps = event->angleDelta().y() / 120;
    if (m_layout.tabs.adjusted(-m_layout.leftArrow.width(), 0, m_layout.rightArrow.width(), 0).contains(pos)) {
        selectCategory(m_category - steps);
    } else {
        m_scroll.wheel(event, true);
        m_hoverItem = itemIndexAt(pos);
        update();
    }
    event->accept();
}

void InventoryWidget::leaveEvent(QEvent* event) {
    m_scroll.leave();
    m_hoverItem = -1;
    m_hoverTab = -1;
    update();
    QWidget::leaveEvent(event);
}
}
