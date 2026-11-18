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
        {"Tools", "tab_tools", {ItemCategory::Tools, ItemCategory::Remembrances}},
        {"Ashes", "tab_ashes", {ItemCategory::SpiritAshes}},
        {"Crafting Materials", "tab_crafting", {ItemCategory::CraftingMaterials}},
        {"Bolstering Materials", "tab_bolstering", {ItemCategory::UpgradeMaterials}},
        {"Key Items", "tab_key_items", {ItemCategory::KeyItems, ItemCategory::GreatRunes,
                                        ItemCategory::CrystalTears}},
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
    constexpr int DESIGN_W = 980;
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

    // The game's own flag (see tools/er_items/gen_items.py): consumables,
    // materials and arrows show their count, gear, spells and unique tools don't,
    // even when holding several. A flask with no charges allocated is stored as
    // its "empty" variant, which shows no number.
    bool showsCount(const InventoryItem& item) {
        return item.baseItem.showsCount;
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
        // Stacks (goods, arrows and bolts) held and stored are one entry; gear and
        // items kept only in the storage chest get their own entry.
        std::unordered_map<uint32_t, size_t> stackRows;
        const auto makeEntry = [](const InventoryItem& item) {
            Entry entry;
            entry.item = item;
            entry.title = fromView(item.baseItem.label);
            if (isWeapon(item)) {
                entry.upgradeLevel = int(item.itemId % 100);
                if (entry.upgradeLevel > 0) entry.title += QString(" +%1").arg(entry.upgradeLevel);
            }
            return entry;
        };
        for (const auto& item : charInfo->inventoryItems) {
            if (item.baseItem.id == 0) continue;
            if (item.baseItem.maxHeld > 0) stackRows.emplace(item.itemId, m_entries.size());
            m_entries.push_back(makeEntry(item));
        }
        for (const auto& item : charInfo->storageItems) {
            if (item.baseItem.id == 0) continue;
            const bool stack = item.baseItem.maxHeld > 0;
            if (stack) {
                if (auto it = stackRows.find(item.itemId); it != stackRows.end()) {
                    m_entries[it->second].stored += item.quantity;
                    continue;
                }
            }
            Entry entry = makeEntry(item);
            entry.held = false;
            entry.stored = item.quantity;
            if (stack) {
                entry.item.quantity = 0;
                stackRows.emplace(item.itemId, m_entries.size());
            }
            m_entries.push_back(std::move(entry));
        }
    }
    for (auto& pane : m_panes) {
        pane.selected = 0;
        pane.hover = -1;
        pane.scroll.reset();
    }
    rebuildVisibleItems();
    update();
}

void InventoryWidget::rebuildVisibleItems() {
    const Tab& tab = TABS[m_category];
    for (int p = 0; p < 2; ++p) {
        Pane& pane = m_panes[p];
        pane.visible.clear();
        for (const auto& entry : m_entries) {
            // Carried items left; stored stacks and pieces kept in the chest right.
            const bool inPane = p == HELD_PANE ? entry.held : (!entry.held || entry.stored > 0);
            if (!inPane) continue;
            if (tab.categories.empty()
                || std::find(tab.categories.begin(), tab.categories.end(), entry.item.baseItem.category)
                   != tab.categories.end()) {
                pane.visible.push_back(&entry);
            }
        }
        if (tab.categories.empty()) {
            // Newest first: the acquisition index grows with every pickup.
            std::stable_sort(pane.visible.begin(), pane.visible.end(), [](const Entry* a, const Entry* b) {
                return a->item.acquisitionIndex > b->item.acquisitionIndex;
            });
        } else {
            std::stable_sort(pane.visible.begin(), pane.visible.end(), [](const Entry* a, const Entry* b) {
                if (a->item.baseItem.sortId != b->item.baseItem.sortId) {
                    return a->item.baseItem.sortId < b->item.baseItem.sortId;
                }
                if (a->item.itemId != b->item.itemId) return a->item.itemId < b->item.itemId;
                return a->item.acquisitionIndex > b->item.acquisitionIndex;
            });
        }
        pane.selected = pane.visible.empty() ? -1 : std::clamp(pane.selected, 0, int(pane.visible.size()) - 1);
    }
    layoutRows();
    updateScrollRange();
}

void InventoryWidget::layoutRows() {
    const bool grouped = !TABS[m_category].categories.empty();
    for (int p = 0; p < 2; ++p) {
        Pane& pane = m_panes[p];
        const auto& visible = pane.visible;
        pane.rows.clear();
        int y = 0;
        for (int i = 0; i < int(visible.size());) {
            Row row;
            row.first = i;
            row.groupStart = grouped && i > 0
                && visible[i]->item.baseItem.sortGroup != visible[i - 1]->item.baseItem.sortGroup;
            if (row.groupStart) y += m_layout.separatorGap;
            row.top = y;
            const uint8_t group = visible[i]->item.baseItem.sortGroup;
            while (i < int(visible.size()) && row.count < m_layout.columns[p]
                   && (!grouped || visible[i]->item.baseItem.sortGroup == group)) {
                ++row.count;
                ++i;
            }
            y += m_layout.rowHeight;
            pane.rows.push_back(row);
        }
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
    for (auto& pane : m_panes) {
        pane.selected = 0;
        pane.hover = -1;
        pane.scroll.reset();
    }
    rebuildVisibleItems();
    update();
}

void InventoryWidget::selectItem(int index) {
    Pane& pane = m_panes[m_activePane];
    const QRect& grid = m_layout.grid[m_activePane];
    if (pane.visible.empty()) return;
    pane.selected = std::clamp(index, 0, int(pane.visible.size()) - 1);
    // Scroll the selected row into view.
    for (const Row& row : pane.rows) {
        if (pane.selected < row.first || pane.selected >= row.first + row.count) continue;
        if (row.top < pane.scroll.value()) pane.scroll.setValue(row.top);
        if (row.top + m_layout.rowHeight > pane.scroll.value() + grid.height()) {
            pane.scroll.setValue(row.top + m_layout.rowHeight - grid.height());
        }
        break;
    }
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

    // Two grids side by side, each with its caption: carried items, then the sort chest.
    // Tiles keep one size; resizing the window adds or removes columns.
    const int captionTop = l.tabs.bottom() + 1 + px(6);
    const int captionH = px(24);
    const int gridTop = captionTop + captionH + px(4);
    const int gap = px(16);
    const int halfW = (inner.width() - gap) / 2;
    const int scrollW = px(14);
    l.cellWidth = TILE_SIZE;
    l.rowHeight = l.cellWidth;
    l.separatorGap = px(8);
    const int barW = std::max(3, px(5));
    l.dividerX = inner.left() + halfW + gap / 2;
    for (int p = 0; p < 2; ++p) {
        const int left = p == HELD_PANE ? inner.left() : inner.right() + 1 - halfW;
        l.caption[p] = QRect(left + px(4), captionTop, halfW - px(8), captionH);
        l.columns[p] = std::max(1, (halfW - px(8) - scrollW) / TILE_SIZE);
        l.grid[p] = QRect(left + px(4), gridTop, l.cellWidth * l.columns[p],
                          std::max(0, l.detail.top() - px(10) - gridTop));
        // At the half's right edge, so it doesn't move when columns come and go.
        l.scrollTrack[p] = QRect(left + halfW - px(4) - barW, gridTop, barW, l.grid[p].height());
    }
    return l;
}

int InventoryWidget::contentHeight(int pane) const {
    const auto& rows = m_panes[pane].rows;
    if (rows.empty()) return 0;
    return rows.back().top + m_layout.rowHeight;
}

void InventoryWidget::updateScrollRange() {
    for (int p = 0; p < 2; ++p) {
        m_panes[p].scroll.setGeometry(m_layout.grid[p], m_layout.scrollTrack[p], contentHeight(p));
        m_panes[p].scroll.setStep(m_layout.rowHeight);
    }
}

void InventoryWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    m_layout = computeLayout();
    keepCategoryVisible();
    m_scaledCache.clear();
    layoutRows();
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
    // The hovered item, else the selection of the pane clicked last.
    for (const Pane& pane : m_panes) {
        if (pane.hover >= 0 && pane.hover < int(pane.visible.size())) return pane.visible[pane.hover];
    }
    const Pane& pane = m_panes[m_activePane];
    if (pane.selected < 0 || pane.selected >= int(pane.visible.size())) return nullptr;
    return pane.visible[pane.selected];
}

QRect InventoryWidget::cellRect(int pane, int row, int column) const {
    const Layout& l = m_layout;
    return QRect(l.grid[pane].left() + column * l.cellWidth,
                 l.grid[pane].top() + m_panes[pane].rows[row].top - m_panes[pane].scroll.value(),
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
    QColor dividerColor = GOLD_COLOR;
    dividerColor.setAlpha(130);
    drawVerticalDivider(painter, m_layout.dividerX, m_layout.caption[HELD_PANE].top(),
                        m_layout.grid[HELD_PANE].bottom(), dividerColor);
    drawGrid(painter, HELD_PANE);
    drawGrid(painter, STORAGE_PANE);
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

void InventoryWidget::drawGrid(QPainter& painter, int p) {
    const Layout& l = m_layout;
    const double s = l.scale;
    const Pane& pane = m_panes[p];
    const QRect& grid = l.grid[p];

    // Caption: the pane's name over a rule, with its item count.
    const QRect caption = l.caption[p];
    painter.setFont(serifFont(int(17 * s)));
    drawText(painter, caption, Qt::AlignLeft | Qt::AlignVCenter, p == HELD_PANE ? "Inventory" : "Sort Chest");
    drawText(painter, caption, Qt::AlignRight | Qt::AlignVCenter, QString::number(pane.visible.size()), DIM_TEXT_COLOR);
    drawRule(painter, caption.left(), caption.right(), caption.bottom(), 110);

    painter.save();
    painter.setClipRect(grid.adjusted(0, 0, 0, 1), Qt::IntersectClip);
    const int cw = l.cellWidth;
    const int rh = l.rowHeight;
    const QSize slotSize(int(cw * SLOT_FILL / SLOT_SPRITE_OPAQUE), int(rh * SLOT_FILL / SLOT_SPRITE_OPAQUE));
    const QPixmap slot = scaled(":/er_images/slot_base", slotSize).scaled(slotSize);
    const QSize cursorSize(int(cw * SLOT_FILL / CURSOR_SPRITE_OPAQUE), int(rh * SLOT_FILL / CURSOR_SPRITE_OPAQUE));
    const QPixmap cursor = scaled(":/er_images/slot_cursor", cursorSize).scaled(cursorSize);
    const QPixmap separator = scaled(":/er_images/group_line", QSize(grid.width(), int(4 * s) + 2))
        .scaled(QSize(grid.width(), std::max(1, int(2 * s))));
    const int markSize = int(cw * 0.2);
    const QPixmap pouchMark = scaled(":/er_images/pouch_mark", QSize(markSize, markSize));
    const QPixmap equipMark = scaled(":/er_images/equip_mark", QSize(markSize, markSize));
    const int iconSize = int(cw * 1.0);
    painter.setFont(serifFont(std::max(10, int(cw * 0.21))));

    for (int r = 0; r < int(pane.rows.size()); ++r) {
        const Row& row = pane.rows[r];
        const int top = grid.top() + row.top - pane.scroll.value();
        if (top > grid.bottom()) break;
        if (row.groupStart) {
            painter.setOpacity(0.55);
            painter.drawPixmap(grid.left(), top - l.separatorGap / 2 - separator.height() / 2, separator);
            painter.setOpacity(1.0);
        }
        if (top + rh < grid.top()) continue;
        for (int col = 0; col < l.columns[p]; ++col) {
            const QRect cell = cellRect(p, r, col);
            const QPoint slotPos(cell.center().x() - slot.width() / 2, cell.center().y() - slot.height() / 2);
            if (col >= row.count) {
                // Rows of a group end with empty, darker slots.
                painter.setOpacity(0.45);
                painter.drawPixmap(slotPos, slot);
                painter.setOpacity(1.0);
                continue;
            }
            const int idx = row.first + col;
            const Entry& entry = *pane.visible[idx];
            painter.drawPixmap(slotPos, slot);
            if (p == m_activePane && idx == pane.selected) {
                painter.drawPixmap(cell.center().x() - cursor.width() / 2, cell.center().y() - cursor.height() / 2, cursor);
            } else if (idx == pane.hover) {
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
                         QString::number(p == STORAGE_PANE ? entry.stored : entry.item.quantity));
            }
        }
    }
    painter.restore();

    // Scroll bar at the panel's right edge.
    pane.scroll.paint(painter, QColor(0, 0, 0, 140), QColor(196, 160, 96), -(l.scrollTrack[p].width() / 4));
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
    QString held = entry->held ? "1 /  1" : "0 /  1";
    QString stored = entry->held ? "- /  -" : "1";
    if (item.baseItem.maxHeld > 0) {
        const uint32_t maxHeld = std::max<uint32_t>(item.baseItem.maxHeld, item.quantity);
        held = QString("%1 /  %2").arg(item.quantity).arg(maxHeld);
        // Flasks and unique items can't go into the chest (stack size 1 there).
        if (item.baseItem.maxStored > 1) {
            stored = QString("%1 /  %2").arg(entry->stored).arg(item.baseItem.maxStored);
        } else if (entry->stored > 0) {
            stored = QString::number(entry->stored);
        } else {
            stored = "- /  -";
        }
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

int InventoryWidget::itemIndexAt(const QPoint& pos, int* pane) const {
    const Layout& l = m_layout;
    for (int p = 0; p < 2; ++p) {
        if (!l.grid[p].contains(pos) || l.cellWidth <= 0) continue;
        const int col = (pos.x() - l.grid[p].left()) / l.cellWidth;
        const int y = pos.y() - l.grid[p].top() + m_panes[p].scroll.value();
        for (const Row& row : m_panes[p].rows) {
            if (y < row.top || y >= row.top + l.rowHeight) continue;
            if (col >= row.count) return -1;
            if (pane != nullptr) *pane = p;
            return row.first + col;
        }
        return -1;
    }
    return -1;
}

void InventoryWidget::setHover(const QPoint& pos) {
    int hoverPane = -1;
    const int hover = m_hasCharacter ? itemIndexAt(pos, &hoverPane) : -1;
    for (int p = 0; p < 2; ++p) {
        const int value = p == hoverPane ? hover : -1;
        if (m_panes[p].hover != value) {
            m_panes[p].hover = value;
            update();
        }
    }
}

int InventoryWidget::tabIndexAt(const QPoint& pos) const {
    const Layout& l = m_layout;
    if (!l.tabs.contains(pos) || l.tabWidth <= 0) return -1;
    const int idx = m_tabOffset + (pos.x() - l.tabs.left()) / l.tabWidth;
    return idx < int(TABS.size()) ? idx : -1;
}

void InventoryWidget::mouseMoveEvent(QMouseEvent* event) {
    for (auto& pane : m_panes) {
        if (pane.scroll.mouseMove(event)) return;
    }
    setHover(event->pos());
    const int tab = m_hasCharacter ? tabIndexAt(event->pos()) : -1;
    if (tab != m_hoverTab) {
        m_hoverTab = tab;
        update();
    }
    setToolTip(tab >= 0 ? TABS[tab].label : QString());
    QWidget::mouseMoveEvent(event);
}

void InventoryWidget::mousePressEvent(QMouseEvent* event) {
    if (m_hasCharacter) {
        for (auto& pane : m_panes) {
            if (pane.scroll.mousePress(event)) return;
        }
    }
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
    } else if (int pane = 0, item = itemIndexAt(pos, &pane); item >= 0) {
        m_activePane = pane;
        m_panes[pane].selected = item;
        update();
    }
}

void InventoryWidget::mouseReleaseEvent(QMouseEvent* event) {
    for (auto& pane : m_panes) {
        if (pane.scroll.mouseRelease(event)) return;
    }
    QWidget::mouseReleaseEvent(event);
}

void InventoryWidget::wheelEvent(QWheelEvent* event) {
    if (!m_hasCharacter) return;
    const QPoint pos = event->position().toPoint();
    const int steps = event->angleDelta().y() / 120;
    if (m_layout.tabs.adjusted(-m_layout.leftButton.width(), 0, m_layout.rightButton.width(), 0).contains(pos)) {
        selectCategory(m_category - steps);
    } else {
        // The half under the pointer scrolls.
        const int p = pos.x() < width() / 2 ? HELD_PANE : STORAGE_PANE;
        m_panes[p].scroll.wheel(event, true);
        setHover(pos);
        update();
    }
    event->accept();
}

void InventoryWidget::keyPressEvent(QKeyEvent* event) {
    if (!m_hasCharacter) return QWidget::keyPressEvent(event);
    switch (event->key()) {
        case Qt::Key_Q: selectCategory(m_category - 1); break;
        case Qt::Key_E: selectCategory(m_category + 1); break;
        // Tab moves the selection to the other grid.
        case Qt::Key_Tab: m_activePane = 1 - m_activePane; update(); break;
        case Qt::Key_Left: selectItem(m_panes[m_activePane].selected - 1); break;
        case Qt::Key_Right: selectItem(m_panes[m_activePane].selected + 1); break;
        case Qt::Key_Up:
        case Qt::Key_Down: {
            // Same column in the previous / next row (rows can be short at group ends).
            const Pane& pane = m_panes[m_activePane];
            if (pane.rows.empty()) break;
            int rowIdx = 0;
            for (int r = 0; r < int(pane.rows.size()); ++r) {
                if (pane.selected >= pane.rows[r].first) rowIdx = r;
            }
            const int col = pane.selected - pane.rows[rowIdx].first;
            const int target = rowIdx + (event->key() == Qt::Key_Up ? -1 : 1);
            if (target < 0 || target >= int(pane.rows.size())) break;
            selectItem(pane.rows[target].first + std::min(col, pane.rows[target].count - 1));
            break;
        }
        default: return QWidget::keyPressEvent(event);
    }
    for (auto& pane : m_panes) pane.hover = -1;
}

void InventoryWidget::leaveEvent(QEvent* event) {
    for (auto& pane : m_panes) {
        pane.scroll.leave();
        pane.hover = -1;
    }
    m_hoverTab = -1;
    update();
    QWidget::leaveEvent(event);
}
}
