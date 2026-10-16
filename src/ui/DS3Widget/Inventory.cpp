#include "Inventory.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QWheelEvent>
#include <algorithm>
#include <array>
#include <cmath>
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
    constexpr int DESIGN_W = 900;
    constexpr int DESIGN_H = 520;
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
    m_souls = charInfo != nullptr ? charInfo->souls : 0;
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
    m_scroll = 0;
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
    m_scroll = std::clamp(m_scroll, 0, maxScroll());
}

void InventoryWidget::selectCategory(int category) {
    category = std::clamp(category, 0, int(CATEGORY_ORDER.size()) - 1);
    if (category == m_category) return;
    m_category = category;
    // Keep the selected tab inside the visible strip.
    if (m_category < m_tabOffset) m_tabOffset = m_category;
    if (m_category >= m_tabOffset + VISIBLE_TABS) m_tabOffset = m_category - VISIBLE_TABS + 1;
    m_scroll = 0;
    m_selectedItem = 0;
    m_hoverItem = -1;
    rebuildVisibleItems();
    update();
}

InventoryWidget::Layout InventoryWidget::computeLayout() const {
    Layout l;
    const QRect r = rect();
    l.scale = std::clamp(std::min(double(r.width()) / DESIGN_W, double(r.height()) / DESIGN_H), 0.6, 1.4);
    const double s = l.scale;
    const auto px = [s](double v) { return int(std::round(v * s)); };

    l.header = QRect(r.left(), r.top(), r.width(), px(46));
    const QRect body = r.adjusted(0, l.header.height() + px(4), 0, 0);
    // The detail panel needs room for the grid next to it.
    const bool showDetail = body.width() >= px(560);
    const int leftWidth = showDetail ? std::max(px(330), body.width() * 46 / 100) : body.width();
    l.leftPanel = QRect(body.left(), body.top(), leftWidth, body.height());
    if (showDetail) {
        l.detailPanel = QRect(l.leftPanel.right() + 1 + px(6), body.top(),
                              body.right() - l.leftPanel.right() - px(6), body.height());
    }

    const QRect inner = l.leftPanel.adjusted(px(14), px(8), -px(14), -px(12));
    l.categoryLabel = QRect(inner.left() + px(8), inner.top(), inner.width() - px(16), px(28));

    const int tabsTop = l.categoryLabel.bottom() + px(8);
    const int tabHeight = px(58);
    const int arrowW = px(26);
    const int tabsWidth = inner.width() - 2 * arrowW;
    l.tabWidth = std::min(tabsWidth / VISIBLE_TABS, int(tabHeight * 1.2));
    const int tabsLeft = inner.left() + (inner.width() - l.tabWidth * VISIBLE_TABS) / 2;
    l.tabs = QRect(tabsLeft, tabsTop, l.tabWidth * VISIBLE_TABS, tabHeight);
    l.leftArrow = QRect(tabsLeft - arrowW, tabsTop, arrowW, tabHeight);
    l.rightArrow = QRect(l.tabs.right() + 1, tabsTop, arrowW, tabHeight);

    l.gridFrame = QRect(inner.left(), l.tabs.bottom() + 1 + px(8), inner.width(),
                        inner.bottom() - l.tabs.bottom() - px(8));
    const QRect frameInner = l.gridFrame.adjusted(px(14), px(8), -px(14), -px(10));
    l.gridTitle = QRect(frameInner.left() + px(8), frameInner.top(), frameInner.width() - px(16), px(26));
    const int gridTop = l.gridTitle.bottom() + 1 + px(10);
    l.cellWidth = frameInner.width() / COLUMNS;
    l.rowHeight = int(l.cellWidth * 1.05);
    l.grid = QRect(frameInner.left() + (frameInner.width() - l.cellWidth * COLUMNS) / 2, gridTop,
                   l.cellWidth * COLUMNS, std::max(0, frameInner.bottom() - gridTop));
    return l;
}

int InventoryWidget::maxScroll() const {
    if (m_layout.rowHeight <= 0) return 0;
    const int rows = int((m_visible.size() + COLUMNS - 1) / COLUMNS);
    return std::max(0, rows * m_layout.rowHeight - m_layout.grid.height());
}

void InventoryWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    m_layout = computeLayout();
    m_scaledCache.clear();
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

    drawHeader(painter);
    drawStonePanel(painter, m_layout.leftPanel);
    if (!m_layout.detailPanel.isEmpty()) drawStonePanel(painter, m_layout.detailPanel);
    if (!m_hasCharacter) {
        painter.setFont(serifFont(int(20 * m_layout.scale)));
        drawShadowText(painter, m_layout.leftPanel, Qt::AlignCenter, "No character selected", DIM_TEXT_COLOR);
        return;
    }
    drawCategories(painter);
    drawGrid(painter);
    drawDetail(painter);
}

void InventoryWidget::drawHeader(QPainter& painter) {
    const Layout& l = m_layout;
    const double s = l.scale;
    const QRect h = l.header;
    const int iconSize = h.height() - int(6 * s);
    const QPixmap icon = scaled(":/ds3_images/menu_invetory", QSize(iconSize, iconSize));
    painter.drawPixmap(h.left() + int(4 * s), h.top() + (h.height() - icon.height()) / 2, icon);
    painter.setFont(serifFont(int(26 * s)));
    drawShadowText(painter, h.adjusted(iconSize + int(14 * s), 0, 0, 0), Qt::AlignLeft | Qt::AlignVCenter, "Inventory");
    if (!m_hasCharacter) return;

    // Souls, top right.
    const int soulsWidth = std::min(int(300 * s), h.width() / 2);
    const QRect souls(h.right() - soulsWidth, h.top() + int(6 * s), soulsWidth - int(8 * s), h.height() - int(14 * s));
    const int soulIcon = int(20 * s);
    painter.drawPixmap(QRect(souls.left(), souls.center().y() - soulIcon / 2, soulIcon, soulIcon),
                       scaled(":/ds3_images/attr_souls", QSize(soulIcon, soulIcon)));
    painter.setFont(serifFont(int(19 * s)));
    drawShadowText(painter, souls.adjusted(soulIcon + int(4 * s), 0, 0, 0), Qt::AlignLeft | Qt::AlignVCenter, "Souls");
    drawShadowText(painter, souls, Qt::AlignRight | Qt::AlignVCenter, QString::number(m_souls));
    drawGoldRule(painter, souls.left(), souls.right() + int(6 * s), souls.bottom() + 2, 200);
}

void InventoryWidget::drawCategories(QPainter& painter) {
    const Layout& l = m_layout;
    const double s = l.scale;
    painter.setFont(serifFont(int(20 * s)));
    drawShadowText(painter, l.categoryLabel.adjusted(int(30 * s), 0, 0, 0), Qt::AlignLeft | Qt::AlignVCenter,
                   categoryLabel(CATEGORY_ORDER[m_category]));
    drawGoldRule(painter, l.categoryLabel.left(), l.categoryLabel.right(), l.categoryLabel.bottom() + 2, 220);

    const int w = l.tabWidth;
    for (int i = 0; i < VISIBLE_TABS; ++i) {
        const int categoryIdx = m_tabOffset + i;
        if (categoryIdx >= int(CATEGORY_ORDER.size())) break;
        const bool selected = categoryIdx == m_category;
        QRect cell(l.tabs.left() + i * w, l.tabs.top(), w, l.tabs.height());
        cell = selected ? cell.adjusted(1, -int(6 * s), -1, int(4 * s)) : cell.adjusted(int(5 * s), int(4 * s), -int(5 * s), -int(2 * s));

        painter.setOpacity(selected ? 1.0 : 0.42);
        painter.drawPixmap(cell, m_tabBackground);
        painter.setOpacity(selected ? 1.0 : 0.72);
        const QString iconName = fromView(fssm::parse::ds3::getCategoryIcon(CATEGORY_ORDER[categoryIdx]));
        const QPixmap icon = scaled(":/ds3_images/" + iconName, cell.size() * (selected ? 0.9 : 0.82));
        painter.drawPixmap(cell.center().x() - icon.width() / 2 + 1, cell.center().y() - icon.height() / 2 + 1, icon);
        painter.setOpacity(1.0);
        if (selected) {
            painter.setPen(QPen(QColor(210, 200, 180, 160), 1));
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

    painter.save();
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(16, 15, 15, 150));
    painter.drawRect(l.gridFrame);
    painter.restore();
    drawOrnateFrame(painter, l.gridFrame, s);

    const Entry* current = currentEntry();
    painter.setFont(serifFont(int(19 * s)));
    drawShadowText(painter, l.gridTitle.adjusted(int(10 * s), 0, 0, 0), Qt::AlignLeft | Qt::AlignVCenter,
                   current != nullptr ? current->title : QString());
    drawGoldRule(painter, l.gridTitle.left(), l.gridTitle.right(), l.gridTitle.bottom() + 2, 200);

    painter.save();
    painter.setClipRect(l.grid, Qt::IntersectClip);
    const int cw = l.cellWidth;
    const int rh = l.rowHeight;
    const int dishW = int(cw * 0.74);
    const QPixmap dish = scaled(":/ds3_images/dish", QSize(dishW, dishW));
    const int iconSize = int(cw * 0.8);
    const int firstRow = m_scroll / rh;
    const int lastRow = (m_scroll + l.grid.height()) / rh;
    painter.setFont(serifFont(std::max(11, int(cw * 0.2))));
    for (int row = firstRow; row <= lastRow; ++row) {
        for (int col = 0; col < COLUMNS; ++col) {
            const int idx = row * COLUMNS + col;
            if (idx >= int(m_visible.size())) break;
            const Entry& entry = *m_visible[idx];
            const QRect cell(l.grid.left() + col * cw, l.grid.top() + row * rh - m_scroll, cw, rh);
            const QRect tile = cell.adjusted(int(3 * s), int(3 * s), -int(3 * s), -int(3 * s));

            // Tile background; the selected one glows orange like in the game.
            QLinearGradient bg(tile.topLeft(), tile.bottomLeft());
            if (idx == m_selectedItem) {
                bg.setColorAt(0.0, QColor(196, 110, 44));
                bg.setColorAt(1.0, QColor(118, 58, 22));
            } else if (idx == m_hoverItem) {
                bg.setColorAt(0.0, QColor(96, 70, 48));
                bg.setColorAt(1.0, QColor(52, 40, 32));
            } else {
                bg.setColorAt(0.0, QColor(40, 38, 38, 200));
                bg.setColorAt(1.0, QColor(26, 25, 25, 200));
            }
            painter.setPen(idx == m_selectedItem ? QPen(QColor(230, 160, 90), 1) : QPen(QColor(0, 0, 0, 120), 1));
            painter.setBrush(bg);
            painter.drawRect(tile.adjusted(0, 0, -1, -1));

            const QPoint dishPos(tile.center().x() - dish.width() / 2, tile.bottom() - dish.height() - int(cw * 0.04));
            painter.drawPixmap(dishPos, dish);
            const QPixmap icon = itemIcon(entry, QSize(iconSize, iconSize));
            const int iconBottom = dishPos.y() + dish.height() * 2 / 3;
            painter.drawPixmap(tile.center().x() - icon.width() / 2, iconBottom - icon.height(), icon);

            if (entry.item.equipped) {
                drawSeal(painter, QPointF(tile.left() + cw * 0.15, tile.top() + cw * 0.15), cw * 0.1);
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
    const int total = maxScroll();
    if (total > 0) {
        const QRect track(l.grid.right() + int(5 * s), l.grid.top(), std::max(2, int(3 * s)), l.grid.height());
        const int thumbH = std::max(16, track.height() * track.height() / (track.height() + total));
        const int thumbY = track.top() + (track.height() - thumbH) * m_scroll / total;
        painter.fillRect(track, QColor(0, 0, 0, 120));
        painter.fillRect(QRect(track.left(), thumbY, track.width(), thumbH), QColor(186, 150, 80));
    }
}

void InventoryWidget::drawDetail(QPainter& painter) {
    const QRect panel = m_layout.detailPanel;
    if (panel.isEmpty()) return;
    const Entry* entry = currentEntry();
    if (entry == nullptr) return;
    const double s = m_layout.scale;
    const auto px = [s](double v) { return int(std::round(v * s)); };
    const QRect inner = panel.adjusted(px(16), px(8), -px(16), -px(12));

    // Name.
    const QRect name(inner.left(), inner.top(), inner.width(), px(30));
    painter.setFont(serifFont(px(20)));
    drawShadowText(painter, name.adjusted(px(4), 0, 0, 0), Qt::AlignLeft | Qt::AlignVCenter, entry->title);
    drawGoldRule(painter, name.left(), name.right(), name.bottom() + 2, 220);

    // Big icon on its dish, top right.
    const int iconSize = std::min(px(130), inner.width() * 2 / 5);
    const QRect iconArea(inner.right() - iconSize - px(6), name.bottom() + px(10), iconSize, iconSize);
    const QPixmap dish = scaled(":/ds3_images/dish", QSize(int(iconSize * 0.72), iconSize));
    const QPoint dishPos(iconArea.center().x() - dish.width() / 2, iconArea.bottom() - dish.height());
    painter.drawPixmap(dishPos, dish);
    const QPixmap icon = itemIcon(*entry, QSize(iconSize, iconSize));
    painter.drawPixmap(iconArea.center().x() - icon.width() / 2, dishPos.y() + dish.height() * 2 / 3 - icon.height(), icon);

    const int textRight = iconArea.left() - px(10);
    const int lineH = px(28);
    painter.setFont(serifFont(px(18)));
    const auto row = [&](int y, const QPixmap& rowIcon, const QString& label, const QString& value) {
        const QRect line(inner.left() + px(4), y, textRight - inner.left() - px(4), lineH);
        int textLeft = line.left();
        if (!rowIcon.isNull()) {
            painter.drawPixmap(line.left(), line.center().y() - rowIcon.height() / 2, rowIcon);
            textLeft += rowIcon.width() + px(4);
        }
        drawShadowText(painter, QRect(textLeft, line.top(), line.right() - textLeft, line.height()),
                       Qt::AlignLeft | Qt::AlignVCenter, label);
        if (!value.isEmpty()) drawShadowText(painter, line, Qt::AlignRight | Qt::AlignVCenter, value);
        drawGoldRule(painter, line.left(), line.right(), line.bottom(), 130);
    };

    const int smallIcon = px(20);
    const ItemCategory category = entry->item.baseItem.category;
    const QString categoryIcon = fromView(fssm::parse::ds3::getCategoryIcon(category));
    int y = name.bottom() + px(10);
    row(y, scaled(":/ds3_images/" + categoryIcon, QSize(smallIcon, smallIcon)), typeLabel(category), {});
    y += lineH + px(26);

    const QPixmap bagIcon = scaled(":/ds3_images/menu_invetory", QSize(smallIcon, smallIcon));
    const QPixmap boxIcon = scaled(":/ds3_images/menu_storage_box", QSize(smallIcon, smallIcon));
    if (entry->maxHeld > 0) {
        row(y, bagIcon, "No. Held", QString("%1/ %2").arg(entry->item.amount).arg(entry->maxHeld));
        y += lineH + px(2);
        const uint16_t maxStored = entry->item.baseItem.max_storage;
        row(y, boxIcon, "Stored", maxStored > 0 ? QString("%1/ %2").arg(entry->stored).arg(maxStored) : "-/   -");
    } else {
        row(y, bagIcon, "No. Held", QString::number(entry->item.amount));
        y += lineH + px(2);
        // Gear in the box can't be told apart from the held piece by id alone.
        row(y, boxIcon, "Stored", "-");
    }
    y = std::max(y + lineH + px(26), iconArea.bottom() + px(16));

    // Reinforcement, infusion and whether the item is equipped.
    const auto [infusion, infusionIcon] = infusionInfo(entry->item.infusion);
    const int wideLeft = inner.left() + px(4);
    const int wideWidth = inner.width() - px(8);
    const auto wideRow = [&](const QString& label, const QString& value) {
        const QRect line(wideLeft, y, wideWidth, lineH);
        drawShadowText(painter, line, Qt::AlignLeft | Qt::AlignVCenter, label);
        drawShadowText(painter, line, Qt::AlignRight | Qt::AlignVCenter, value);
        drawGoldRule(painter, line.left(), line.right(), line.bottom(), 130);
        y += lineH + px(2);
    };
    const bool isEquipment = entry->maxHeld == 0 && category != ItemCategory::KeyItems
        && category != ItemCategory::Spells && category != ItemCategory::CovenantItem;
    if (isEquipment) {
        wideRow("Reinforcement", entry->item.upgradeLevel > 0 ? QString("+%1").arg(entry->item.upgradeLevel) : "-");
    }
    if (!infusion.isEmpty()) wideRow("Infusion", infusion);
    if (entry->item.equipped) {
        drawSeal(painter, QPointF(wideLeft + smallIcon / 2.0, y + lineH / 2.0), smallIcon * 0.4);
        drawShadowText(painter, QRect(wideLeft + smallIcon + px(4), y, wideWidth - smallIcon - px(4), lineH),
                       Qt::AlignLeft | Qt::AlignVCenter, "Equipped");
    }
}

int InventoryWidget::itemIndexAt(const QPoint& pos) const {
    const Layout& l = m_layout;
    if (!l.grid.contains(pos) || l.cellWidth <= 0) return -1;
    const int col = (pos.x() - l.grid.left()) / l.cellWidth;
    const int row = (pos.y() - l.grid.top() + m_scroll) / l.rowHeight;
    if (col < 0 || col >= COLUMNS) return -1;
    const int idx = row * COLUMNS + col;
    return idx < int(m_visible.size()) ? idx : -1;
}

int InventoryWidget::tabIndexAt(const QPoint& pos) const {
    const Layout& l = m_layout;
    if (!l.tabs.contains(pos) || l.tabWidth <= 0) return -1;
    const int idx = m_tabOffset + (pos.x() - l.tabs.left()) / l.tabWidth;
    return idx < int(CATEGORY_ORDER.size()) ? idx : -1;
}

void InventoryWidget::mouseMoveEvent(QMouseEvent* event) {
    const int hover = m_hasCharacter ? itemIndexAt(event->pos()) : -1;
    if (hover != m_hoverItem) {
        m_hoverItem = hover;
        update();
    }
    const int tab = m_hasCharacter ? tabIndexAt(event->pos()) : -1;
    setToolTip(tab >= 0 ? categoryLabel(CATEGORY_ORDER[tab]) : QString());
    QWidget::mouseMoveEvent(event);
}

void InventoryWidget::mousePressEvent(QMouseEvent* event) {
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

void InventoryWidget::wheelEvent(QWheelEvent* event) {
    if (!m_hasCharacter) return;
    const QPoint pos = event->position().toPoint();
    const int steps = event->angleDelta().y() / 120;
    if (m_layout.tabs.adjusted(-m_layout.leftArrow.width(), 0, m_layout.rightArrow.width(), 0).contains(pos)) {
        selectCategory(m_category - steps);
    } else {
        m_scroll = std::clamp(m_scroll - steps * m_layout.rowHeight, 0, maxScroll());
        m_hoverItem = itemIndexAt(pos);
        update();
    }
    event->accept();
}

void InventoryWidget::leaveEvent(QEvent* event) {
    m_hoverItem = -1;
    update();
    QWidget::leaveEvent(event);
}
}
