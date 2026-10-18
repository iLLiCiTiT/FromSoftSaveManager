#include "Inventory.h"

#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QWheelEvent>
#include <algorithm>
#include <array>
#include <cmath>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "../Utils.h"

namespace fssm::ui::er {
using fssm::parse::er::InventoryItem;
using fssm::parse::er::ItemCategory;

namespace {
    struct Tab {
        const char* label;          // GR_MenuText 40500..40524
        const char* icon;           // :/er_images/<icon>
        std::vector<ItemCategory> categories;  // empty = Recent Items
    };

    // Same order as the in-game inventory tabs.
    const std::array<Tab, 19> TABS = {{
        {"Recent Items", "tab_recent", {}},
        {"Tools", "tab_tools", {ItemCategory::Tools}},
        {"Ashes", "tab_ashes", {ItemCategory::SpiritAshes}},
        {"Crafting Materials", "tab_crafting", {ItemCategory::CraftingMaterials}},
        {"Bolstering Materials", "tab_bolstering", {ItemCategory::UpgradeMaterials}},
        {"Key Items", "tab_key_items", {ItemCategory::KeyItems, ItemCategory::Remembrances,
                                        ItemCategory::GreatRunes, ItemCategory::CrystalTears}},
        {"Sorceries", "tab_sorceries", {ItemCategory::Sorceries}},
        {"Incantations", "tab_incantations", {ItemCategory::Incantations}},
        {"Ashes of War", "tab_ashes_of_war", {ItemCategory::AshesOfWar}},
        {"Melee Armaments", "tab_melee", {ItemCategory::MeleeWeapons}},
        {"Ranged Weapons / Catalysts", "tab_ranged", {ItemCategory::RangedWeapons, ItemCategory::Catalysts}},
        {"Arrows/Bolts", "tab_ammo", {ItemCategory::Ammunition}},
        {"Shields", "tab_shields", {ItemCategory::Shields}},
        {"Head", "tab_head", {ItemCategory::HeadArmor}},
        {"Chest", "tab_chest", {ItemCategory::ChestArmor}},
        {"Arms", "tab_arms", {ItemCategory::ArmsArmor}},
        {"Legs", "tab_legs", {ItemCategory::LegsArmor}},
        {"Talismans", "tab_talismans", {ItemCategory::Talismans}},
        {"Info", "tab_info", {ItemCategory::Info}},
    }};

    // Reference layout size; everything scales with the widget.
    constexpr int DESIGN_W = 560;
    constexpr int DESIGN_H = 470;
    // Tabs, padding, fonts and the detail panel keep one size (the look at the default
    // window size); only the item grid follows the widget size.
    constexpr double FIXED_SCALE = 0.76;
    // Item tile pitch in pixels, independent of the widget size; the slot
    // background fills all of it but a small gap.
    constexpr int TILE_SIZE = 60;
    constexpr double SLOT_FILL = 0.95;
    // Opaque part of the slot / cursor sprites (they have soft transparent margins).
    constexpr double SLOT_SPRITE_OPAQUE = 0.86;
    constexpr double CURSOR_SPRITE_OPAQUE = 0.69;

    const QColor TEXT_COLOR(228, 224, 210);
    const QColor DIM_TEXT_COLOR(150, 144, 128);
    const QColor GOLD_COLOR(190, 168, 112);

    QFont serifFont(int pixelSize) {
        QFont font("Georgia");
        font.setStyleHint(QFont::Serif);
        font.setPixelSize(std::max(8, pixelSize));
        return font;
    }

    void drawText(QPainter& painter, const QRect& rect, int flags, const QString& text,
                  const QColor& color = TEXT_COLOR) {
        const QFontMetrics fm(painter.font());
        const QString elided = (flags & Qt::TextWordWrap) ? text : fm.elidedText(text, Qt::ElideRight, rect.width());
        const int offset = std::max(1, painter.font().pixelSize() / 14);
        painter.save();
        painter.setPen(QColor(0, 0, 0, 210));
        painter.drawText(rect.translated(offset, offset), flags, elided);
        painter.setPen(color);
        painter.drawText(rect, flags, elided);
        painter.restore();
    }

    // Thin rule that fades out to the right, like the lines under the game's headings.
    void drawRule(QPainter& painter, int x1, int x2, int y, int alpha = 150) {
        if (x2 <= x1) return;
        QLinearGradient gradient(x1, 0, x2, 0);
        QColor color = GOLD_COLOR;
        color.setAlpha(alpha);
        QColor clear = color;
        clear.setAlpha(0);
        gradient.setColorAt(0.0, color);
        gradient.setColorAt(0.7, color);
        gradient.setColorAt(1.0, clear);
        painter.save();
        painter.setPen(QPen(QBrush(gradient), 1));
        painter.drawLine(x1, y, x2, y);
        painter.restore();
    }

    // Translucent dark backdrop the game puts behind the inventory panes.
    void drawPane(QPainter& painter, const QRect& rect) {
        painter.save();
        QLinearGradient gradient(rect.topLeft(), rect.topRight());
        gradient.setColorAt(0.0, QColor(10, 10, 8, 150));
        gradient.setColorAt(0.85, QColor(10, 10, 8, 110));
        gradient.setColorAt(1.0, QColor(10, 10, 8, 40));
        painter.setPen(Qt::NoPen);
        painter.setBrush(gradient);
        painter.drawRect(rect);
        painter.restore();
    }

    QString fromView(std::string_view text) {
        return QString::fromUtf8(text.data(), int(text.size()));
    }

    bool isWeapon(const InventoryItem& item) {
        return (item.itemId >> 28) == 0 && item.baseItem.category != ItemCategory::Ammunition;
    }

    // Goods that stack show their count in the grid; gear and key items don't.
    bool showsCount(const InventoryItem& item) {
        const ItemCategory category = item.baseItem.category;
        if (category == ItemCategory::Tools || category == ItemCategory::Ammunition) return true;
        return item.baseItem.maxHeld > 1;
    }
}

InventoryWidget::InventoryWidget(QWidget* parent): QWidget(parent) {
    setAttribute(Qt::WA_TranslucentBackground, true);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
}

QSize InventoryWidget::sizeHint() const {
    return {DESIGN_W, DESIGN_H};
}

void InventoryWidget::setCharacter(const fssm::parse::er::ERCharacterInfo* charInfo) {
    m_entries.clear();
    m_hasCharacter = charInfo != nullptr;
    if (charInfo != nullptr) {
        std::unordered_map<uint32_t, uint32_t> stored;
        for (const auto& item : charInfo->storageItems) {
            if ((item.itemId >> 28) == 0x4) stored[item.itemId] += item.quantity;
        }
        for (const auto& item : charInfo->inventoryItems) {
            if (item.baseItem.id == 0) continue;
            Entry entry;
            entry.item = item;
            entry.title = fromView(item.baseItem.label);
            if (isWeapon(item)) {
                entry.upgradeLevel = int(item.itemId % 100);
                if (entry.upgradeLevel > 0) entry.title += QString(" +%1").arg(entry.upgradeLevel);
            }
            if (auto it = stored.find(item.itemId); it != stored.end()) entry.stored = it->second;
            m_entries.push_back(std::move(entry));
        }
    }
    m_selectedItem = 0;
    m_hoverItem = -1;
    m_scroll = 0;
    rebuildVisibleItems();
    update();
}

void InventoryWidget::rebuildVisibleItems() {
    m_visible.clear();
    const Tab& tab = TABS[m_category];
    if (tab.categories.empty()) {
        for (const auto& entry : m_entries) m_visible.push_back(&entry);
        // Newest first: the acquisition index grows with every pickup.
        std::stable_sort(m_visible.begin(), m_visible.end(), [](const Entry* a, const Entry* b) {
            return a->item.acquisitionIndex > b->item.acquisitionIndex;
        });
    } else {
        for (const auto& entry : m_entries) {
            if (std::find(tab.categories.begin(), tab.categories.end(), entry.item.baseItem.category)
                != tab.categories.end()) {
                m_visible.push_back(&entry);
            }
        }
        std::stable_sort(m_visible.begin(), m_visible.end(), [](const Entry* a, const Entry* b) {
            if (a->item.baseItem.sortId != b->item.baseItem.sortId) {
                return a->item.baseItem.sortId < b->item.baseItem.sortId;
            }
            if (a->item.itemId != b->item.itemId) return a->item.itemId < b->item.itemId;
            return a->item.acquisitionIndex > b->item.acquisitionIndex;
        });
    }
    layoutRows();
    m_selectedItem = m_visible.empty() ? -1 : std::clamp(m_selectedItem, 0, int(m_visible.size()) - 1);
    m_scroll = std::clamp(m_scroll, 0, maxScroll());
}

void InventoryWidget::layoutRows() {
    m_rows.clear();
    const bool grouped = !TABS[m_category].categories.empty();
    int y = 0;
    for (int i = 0; i < int(m_visible.size());) {
        Row row;
        row.first = i;
        row.groupStart = grouped && i > 0
            && m_visible[i]->item.baseItem.sortGroup != m_visible[i - 1]->item.baseItem.sortGroup;
        if (row.groupStart) y += m_layout.separatorGap;
        row.top = y;
        const uint8_t group = m_visible[i]->item.baseItem.sortGroup;
        while (i < int(m_visible.size()) && row.count < m_layout.columns
               && (!grouped || m_visible[i]->item.baseItem.sortGroup == group)) {
            ++row.count;
            ++i;
        }
        y += m_layout.rowHeight;
        m_rows.push_back(row);
    }
}

void InventoryWidget::keepCategoryVisible() {
    const int visible = std::max(1, m_layout.visibleTabs);
    if (m_category < m_tabOffset) m_tabOffset = m_category;
    if (m_category >= m_tabOffset + visible) m_tabOffset = m_category - visible + 1;
    m_tabOffset = std::clamp(m_tabOffset, 0, std::max(0, int(TABS.size()) - visible));
}

void InventoryWidget::selectCategory(int category) {
    category = std::clamp(category, 0, int(TABS.size()) - 1);
    if (category == m_category) return;
    m_category = category;
    keepCategoryVisible();
    m_scroll = 0;
    m_selectedItem = 0;
    m_hoverItem = -1;
    rebuildVisibleItems();
    update();
}

void InventoryWidget::selectItem(int index) {
    if (m_visible.empty()) return;
    m_selectedItem = std::clamp(index, 0, int(m_visible.size()) - 1);
    // Scroll the selected row into view.
    for (int r = 0; r < int(m_rows.size()); ++r) {
        const Row& row = m_rows[r];
        if (m_selectedItem < row.first || m_selectedItem >= row.first + row.count) continue;
        if (row.top < m_scroll) m_scroll = row.top;
        if (row.top + m_layout.rowHeight > m_scroll + m_layout.grid.height()) {
            m_scroll = row.top + m_layout.rowHeight - m_layout.grid.height();
        }
        break;
    }
    m_scroll = std::clamp(m_scroll, 0, maxScroll());
    update();
}

InventoryWidget::Layout InventoryWidget::computeLayout() const {
    Layout l;
    const QRect r = rect();
    l.scale = FIXED_SCALE;
    const double s = l.scale;
    const auto px = [s](double v) { return int(std::round(v * s)); };

    l.panel = r;
    const QRect inner = r.adjusted(px(12), px(8), -px(12), -px(10));

    // Category tabs (the name shows as a tooltip); a wider panel shows more of them.
    const int tabsTop = inner.top() + px(4);
    const int tabHeight = px(56);
    const int buttonW = px(34);
    const int tabsWidth = inner.width() - 2 * (buttonW + px(4));
    const int maxTabWidth = int(tabHeight * 0.95);
    l.visibleTabs = std::clamp(tabsWidth / maxTabWidth, 5, int(TABS.size()));
    l.tabWidth = std::min(tabsWidth / l.visibleTabs, maxTabWidth);
    const int tabsLeft = inner.left() + (inner.width() - l.tabWidth * l.visibleTabs) / 2;
    l.tabs = QRect(tabsLeft, tabsTop, l.tabWidth * l.visibleTabs, tabHeight);
    l.leftButton = QRect(tabsLeft - buttonW - px(4), tabsTop, buttonW, tabHeight);
    l.rightButton = QRect(l.tabs.right() + 1 + px(4), tabsTop, buttonW, tabHeight);

    // Selected item details along the bottom.
    const int detailHeight = px(104);
    l.detail = QRect(inner.left(), inner.bottom() - detailHeight + 1, inner.width(), detailHeight);

    // Tiles keep one size; resizing the window adds or removes columns.
    const int gridTop = l.tabs.bottom() + 1 + px(10);
    const int scrollW = px(14);
    const int gridWidth = inner.width() - px(8) - scrollW;
    l.cellWidth = TILE_SIZE;
    l.columns = std::max(1, gridWidth / TILE_SIZE);
    l.rowHeight = l.cellWidth;
    l.separatorGap = px(8);
    l.grid = QRect(inner.left() + px(4), gridTop, l.cellWidth * l.columns,
                   std::max(0, l.detail.top() - px(10) - gridTop));
    return l;
}

int InventoryWidget::contentHeight() const {
    if (m_rows.empty()) return 0;
    return m_rows.back().top + m_layout.rowHeight;
}

int InventoryWidget::maxScroll() const {
    return std::max(0, contentHeight() - m_layout.grid.height());
}

void InventoryWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    m_layout = computeLayout();
    keepCategoryVisible();
    m_scaledCache.clear();
    layoutRows();
    m_scroll = std::clamp(m_scroll, 0, maxScroll());
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
    const QString image = fromView(entry.item.baseItem.image);
    if (inventoryResourcesAvailable() && !image.isEmpty()) icon = scaled(":/er_inv_images/" + image, size);
    if (icon.isNull()) {
        // Without the icon bundle, fall back to the tab icon of the item's category.
        for (const Tab& tab : TABS) {
            if (std::find(tab.categories.begin(), tab.categories.end(), entry.item.baseItem.category)
                != tab.categories.end()) {
                icon = scaled(QString(":/er_images/") + tab.icon, size * 0.7);
                break;
            }
        }
    }
    return icon;
}

const InventoryWidget::Entry* InventoryWidget::currentEntry() const {
    const int idx = m_hoverItem >= 0 ? m_hoverItem : m_selectedItem;
    if (idx < 0 || idx >= int(m_visible.size())) return nullptr;
    return m_visible[idx];
}

QRect InventoryWidget::cellRect(int row, int column) const {
    const Layout& l = m_layout;
    return QRect(l.grid.left() + column * l.cellWidth, l.grid.top() + m_rows[row].top - m_scroll,
                 l.cellWidth, l.rowHeight);
}

void InventoryWidget::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    painter.setClipRect(event->rect());
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);
    if (m_layout.cellWidth == 0) {
        m_layout = computeLayout();
        layoutRows();
    }

    drawPane(painter, m_layout.panel);
    if (!m_hasCharacter) {
        painter.setFont(serifFont(int(20 * m_layout.scale)));
        drawText(painter, m_layout.panel, Qt::AlignCenter, "No character selected", DIM_TEXT_COLOR);
        return;
    }
    drawTabs(painter);
    drawGrid(painter);
    drawDetail(painter);
}

void InventoryWidget::drawTabs(QPainter& painter) {
    const Layout& l = m_layout;
    const double s = l.scale;
    // Strip behind the tabs.
    const QRect strip(l.leftButton.left(), l.tabs.top(), l.rightButton.right() - l.leftButton.left(), l.tabs.height());
    painter.save();
    painter.setOpacity(0.8);
    painter.drawPixmap(strip, scaled(":/er_images/tab_base", strip.size() * 1.0).scaled(strip.size()));
    painter.restore();

    const int w = l.tabWidth;
    for (int i = 0; i < l.visibleTabs; ++i) {
        const int tabIdx = m_tabOffset + i;
        if (tabIdx >= int(TABS.size())) break;
        const bool selected = tabIdx == m_category;
        const bool hovered = !selected && tabIdx == m_hoverTab;
        const QRect cell(l.tabs.left() + i * w, l.tabs.top(), w, l.tabs.height());
        if (selected) {
            const QSize cursorSize(int(w * 1.3), int(l.tabs.height() * 1.25));
            const QPixmap cursor = scaled(":/er_images/tab_cursor", cursorSize).scaled(cursorSize);
            painter.drawPixmap(cell.center().x() - cursor.width() / 2, cell.center().y() - cursor.height() / 2 + int(2 * s),
                               cursor);
        }
        if (hovered) {
            const QSize cursorSize(int(w * 1.3), int(l.tabs.height() * 1.25));
            painter.setOpacity(0.35);
            painter.drawPixmap(cell.center().x() - cursorSize.width() / 2,
                               cell.center().y() - cursorSize.height() / 2 + int(2 * s),
                               scaled(":/er_images/tab_cursor", cursorSize).scaled(cursorSize));
        }
        painter.setOpacity(selected ? 1.0 : hovered ? 0.85 : 0.55);
        const QPixmap icon = scaled(QString(":/er_images/") + TABS[tabIdx].icon, cell.size() * 0.9);
        painter.drawPixmap(cell.center().x() - icon.width() / 2, cell.center().y() - icon.height() / 2, icon);
        painter.setOpacity(1.0);
    }

    // Previous / next category arrows on both sides of the strip (the game's
    // menu chevron; the right one mirrored).
    const QPixmap arrow = scaled(":/er_images/arrow", QSize(l.leftButton.width(), int(l.tabs.height() * 0.5)));
    const QPixmap rightArrow = arrow.transformed(QTransform().scale(-1, 1));
    for (const auto& [rect, pix, enabled] : {
             std::tuple{l.leftButton, arrow, m_category > 0},
             std::tuple{l.rightButton, rightArrow, m_category + 1 < int(TABS.size())}}) {
        painter.setOpacity(enabled ? 1.0 : 0.3);
        painter.drawPixmap(rect.center().x() - pix.width() / 2, rect.center().y() - pix.height() / 2, pix);
    }
    painter.setOpacity(1.0);
}

void InventoryWidget::drawGrid(QPainter& painter) {
    const Layout& l = m_layout;
    const double s = l.scale;

    painter.save();
    painter.setClipRect(l.grid.adjusted(0, 0, 0, 1), Qt::IntersectClip);
    const int cw = l.cellWidth;
    const int rh = l.rowHeight;
    const QSize slotSize(int(cw * SLOT_FILL / SLOT_SPRITE_OPAQUE), int(rh * SLOT_FILL / SLOT_SPRITE_OPAQUE));
    const QPixmap slot = scaled(":/er_images/slot_base", slotSize).scaled(slotSize);
    const QSize cursorSize(int(cw * SLOT_FILL / CURSOR_SPRITE_OPAQUE), int(rh * SLOT_FILL / CURSOR_SPRITE_OPAQUE));
    const QPixmap cursor = scaled(":/er_images/slot_cursor", cursorSize).scaled(cursorSize);
    const QPixmap separator = scaled(":/er_images/group_line", QSize(l.grid.width(), int(4 * s) + 2))
        .scaled(QSize(l.grid.width(), std::max(1, int(2 * s))));
    const int markSize = int(cw * 0.2);
    const QPixmap pouchMark = scaled(":/er_images/pouch_mark", QSize(markSize, markSize));
    const QPixmap equipMark = scaled(":/er_images/equip_mark", QSize(markSize, markSize));
    const int iconSize = int(cw * 1.0);
    painter.setFont(serifFont(std::max(10, int(cw * 0.21))));

    for (int r = 0; r < int(m_rows.size()); ++r) {
        const Row& row = m_rows[r];
        const int top = l.grid.top() + row.top - m_scroll;
        if (top > l.grid.bottom()) break;
        if (row.groupStart) {
            painter.setOpacity(0.55);
            painter.drawPixmap(l.grid.left(), top - l.separatorGap / 2 - separator.height() / 2, separator);
            painter.setOpacity(1.0);
        }
        if (top + rh < l.grid.top()) continue;
        for (int col = 0; col < l.columns; ++col) {
            const QRect cell = cellRect(r, col);
            const QPoint slotPos(cell.center().x() - slot.width() / 2, cell.center().y() - slot.height() / 2);
            if (col >= row.count) {
                // Rows of a group end with empty, darker slots.
                painter.setOpacity(0.45);
                painter.drawPixmap(slotPos, slot);
                painter.setOpacity(1.0);
                continue;
            }
            const int idx = row.first + col;
            const Entry& entry = *m_visible[idx];
            painter.drawPixmap(slotPos, slot);
            if (idx == m_selectedItem) {
                painter.drawPixmap(cell.center().x() - cursor.width() / 2, cell.center().y() - cursor.height() / 2, cursor);
            } else if (idx == m_hoverItem) {
                painter.setOpacity(0.4);
                painter.drawPixmap(cell.center().x() - cursor.width() / 2, cell.center().y() - cursor.height() / 2, cursor);
                painter.setOpacity(1.0);
            }
            const QPixmap icon = itemIcon(entry, QSize(iconSize, iconSize));
            painter.drawPixmap(cell.center().x() - icon.width() / 2, cell.center().y() - icon.height() / 2, icon);

            if (entry.item.inPouch) {
                painter.drawPixmap(cell.left() + int(cw * 0.08), cell.top() + int(cw * 0.06), pouchMark);
            } else if (entry.item.equipped) {
                painter.drawPixmap(cell.left() + int(cw * 0.06), cell.top() + int(cw * 0.06), equipMark);
            }
            if (showsCount(entry.item)) {
                drawText(painter, cell.adjusted(0, 0, -int(cw * 0.1), -int(cw * 0.06)), Qt::AlignRight | Qt::AlignBottom,
                         QString::number(entry.item.quantity));
            }
        }
    }
    painter.restore();

    // Scroll bar right of the grid.
    const int total = maxScroll();
    if (total > 0) {
        const QRect track(l.grid.right() + int(8 * s), l.grid.top(), std::max(3, int(5 * s)), l.grid.height());
        const int thumbH = std::max(16, track.height() * track.height() / (track.height() + total));
        const int thumbY = track.top() + (track.height() - thumbH) * m_scroll / total;
        painter.fillRect(track, QColor(0, 0, 0, 140));
        painter.fillRect(QRect(track.left() + track.width() / 4, thumbY, std::max(1, track.width() / 2), thumbH),
                         QColor(196, 160, 96));
    }
}

void InventoryWidget::drawDetail(QPainter& painter) {
    const QRect area = m_layout.detail;
    const double s = m_layout.scale;
    const auto px = [s](double v) { return int(std::round(v * s)); };
    painter.save();
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(10, 10, 8, 120));
    painter.drawRect(area);
    painter.restore();
    drawRule(painter, area.left(), area.right(), area.top(), 150);
    const Entry* entry = currentEntry();
    if (entry == nullptr) return;
    const InventoryItem& item = entry->item;

    // Large icon in its frame at the left.
    const int frameH = area.height() - px(8);
    const QRect frame(area.left() + px(4), area.top() + px(4), int(frameH * 0.95), frameH);
    painter.drawPixmap(frame, scaled(":/er_images/item_frame", frame.size()).scaled(frame.size()));
    const QPixmap icon = itemIcon(*entry, frame.size() * 0.82);
    painter.drawPixmap(frame.center().x() - icon.width() / 2, frame.center().y() - icon.height() / 2, icon);

    // Text columns: name and effect, No. Held / Stored, then upgrade and status.
    // When narrow, upgrade and status go under the counts.
    const QRect text(frame.right() + px(14), area.top() + px(6), area.right() - frame.right() - px(18),
                     area.height() - px(12));
    const bool wide = text.width() >= px(560);
    const int lineH = text.height() / 3;
    const int col1W = text.width() * (wide ? 44 : 52) / 100;
    const int col2W = wide ? text.width() * 28 / 100 : text.width() - col1W;
    const int gap = px(16);
    const QRect col1(text.left(), text.top(), col1W - gap, text.height());
    const QRect col2(text.left() + col1W, text.top(), col2W - (wide ? gap : 0), text.height());
    const QRect col3(text.left() + col1W + col2W, text.top(), text.width() - col1W - col2W, text.height());
    const auto line = [&](const QRect& col, int row) {
        return QRect(col.left(), col.top() + row * lineH, col.width(), lineH);
    };
    const auto valueRow = [&](const QRect& col, int row, const QString& label, const QString& value) {
        const QRect r = line(col, row);
        drawText(painter, r, Qt::AlignLeft | Qt::AlignVCenter, label);
        if (!value.isEmpty()) drawText(painter, r, Qt::AlignRight | Qt::AlignVCenter, value);
        drawRule(painter, r.left(), r.right(), r.bottom(), 90);
    };

    painter.setFont(serifFont(px(19)));
    drawText(painter, line(col1, 0), Qt::AlignLeft | Qt::AlignVCenter, entry->title);
    painter.setFont(serifFont(px(15)));
    const QString effect = fromView(item.baseItem.effect);
    if (!effect.isEmpty()) {
        const QRect effectRect(col1.left(), col1.top() + lineH, col1.width(), 2 * lineH);
        drawText(painter, effectRect, Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap, effect, DIM_TEXT_COLOR.lighter(125));
    }

    // Counts.
    QString held = "1 /  1";
    QString stored = "- /  -";
    if (item.baseItem.maxHeld > 0) {
        const uint32_t maxHeld = std::max<uint32_t>(item.baseItem.maxHeld, item.quantity);
        held = QString("%1 /  %2").arg(item.quantity).arg(maxHeld);
        // Flasks and unique items can't go into the chest (stack size 1 there).
        if (item.baseItem.maxStored > 1) stored = QString("%1 /  %2").arg(entry->stored).arg(item.baseItem.maxStored);
    }
    valueRow(col2, 0, "No. Held", held);
    valueRow(col2, 1, "Stored", stored);

    // Upgrade level and equipped / pouch status.
    const QRect statusCol = wide ? col3 : col2;
    int row = wide ? 0 : 2;
    if (isWeapon(item) && row < 3) {
        valueRow(statusCol, row++, "Upgrade", entry->upgradeLevel > 0 ? QString("+%1").arg(entry->upgradeLevel) : "-");
    }
    if ((item.equipped || item.inPouch) && row < 3) {
        const QRect r = line(statusCol, row);
        const int mark = std::min(px(20), lineH);
        painter.drawPixmap(r.left(), r.center().y() - mark / 2,
                           scaled(item.inPouch ? ":/er_images/pouch_mark" : ":/er_images/equip_mark", QSize(mark, mark)));
        drawText(painter, r.adjusted(mark + px(6), 0, 0, 0), Qt::AlignLeft | Qt::AlignVCenter,
                 item.inPouch ? "In pouch / quick items" : "Equipped");
    }
}

int InventoryWidget::itemIndexAt(const QPoint& pos) const {
    const Layout& l = m_layout;
    if (!l.grid.contains(pos) || l.cellWidth <= 0) return -1;
    const int col = (pos.x() - l.grid.left()) / l.cellWidth;
    const int y = pos.y() - l.grid.top() + m_scroll;
    for (const Row& row : m_rows) {
        if (y < row.top || y >= row.top + l.rowHeight) continue;
        return col < row.count ? row.first + col : -1;
    }
    return -1;
}

int InventoryWidget::tabIndexAt(const QPoint& pos) const {
    const Layout& l = m_layout;
    if (!l.tabs.contains(pos) || l.tabWidth <= 0) return -1;
    const int idx = m_tabOffset + (pos.x() - l.tabs.left()) / l.tabWidth;
    return idx < int(TABS.size()) ? idx : -1;
}

void InventoryWidget::mouseMoveEvent(QMouseEvent* event) {
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
    setToolTip(tab >= 0 ? TABS[tab].label : QString());
    QWidget::mouseMoveEvent(event);
}

void InventoryWidget::mousePressEvent(QMouseEvent* event) {
    if (!m_hasCharacter || event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }
    const QPoint pos = event->pos();
    if (m_layout.leftButton.contains(pos)) {
        selectCategory(m_category - 1);
    } else if (m_layout.rightButton.contains(pos)) {
        selectCategory(m_category + 1);
    } else if (const int tab = tabIndexAt(pos); tab >= 0) {
        selectCategory(tab);
    } else if (const int item = itemIndexAt(pos); item >= 0) {
        m_selectedItem = item;
        update();
    }
}

void InventoryWidget::wheelEvent(QWheelEvent* event) {
    if (!m_hasCharacter) return;
    const QPoint pos = event->position().toPoint();
    const int steps = event->angleDelta().y() / 120;
    if (m_layout.tabs.adjusted(-m_layout.leftButton.width(), 0, m_layout.rightButton.width(), 0).contains(pos)) {
        selectCategory(m_category - steps);
    } else {
        m_scroll = std::clamp(m_scroll - steps * m_layout.rowHeight, 0, maxScroll());
        m_hoverItem = itemIndexAt(pos);
        update();
    }
    event->accept();
}

void InventoryWidget::keyPressEvent(QKeyEvent* event) {
    if (!m_hasCharacter) return QWidget::keyPressEvent(event);
    switch (event->key()) {
        case Qt::Key_Q: selectCategory(m_category - 1); break;
        case Qt::Key_E: selectCategory(m_category + 1); break;
        case Qt::Key_Left: selectItem(m_selectedItem - 1); break;
        case Qt::Key_Right: selectItem(m_selectedItem + 1); break;
        case Qt::Key_Up:
        case Qt::Key_Down: {
            // Same column in the previous / next row (rows can be short at group ends).
            int rowIdx = 0;
            for (int r = 0; r < int(m_rows.size()); ++r) {
                if (m_selectedItem >= m_rows[r].first) rowIdx = r;
            }
            const int col = m_selectedItem - m_rows[rowIdx].first;
            const int target = rowIdx + (event->key() == Qt::Key_Up ? -1 : 1);
            if (target < 0 || target >= int(m_rows.size())) break;
            selectItem(m_rows[target].first + std::min(col, m_rows[target].count - 1));
            break;
        }
        default: return QWidget::keyPressEvent(event);
    }
    m_hoverItem = -1;
}

void InventoryWidget::leaveEvent(QEvent* event) {
    m_hoverItem = -1;
    m_hoverTab = -1;
    update();
    QWidget::leaveEvent(event);
}
}
