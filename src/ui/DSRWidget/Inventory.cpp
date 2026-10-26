#include "Inventory.h"

#include <QMouseEvent>
#include <QPaintEvent>
#include <QRegularExpression>
#include <QPainter>
#include <QWheelEvent>
#include <algorithm>
#include <array>
#include <cmath>
#include <set>
#include <unordered_map>

#include "Painting.h"
#include "../Utils.h"

namespace fssm::ui::dsr {
namespace {
    using fssm::parse::dsr::ItemCategory;

    // Sizes are measured on a 1440p capture of the game's inventory; FIXED_SCALE maps
    // them to the app. Rows, icons and fonts keep this one size; resizing only changes
    // how much fits.
    constexpr double FIXED_SCALE = 0.5;
    constexpr uint32_t EMPTY_SLOT = 0xFFFFFFFFu;

    // The game's tab order.
    constexpr std::array<ItemCategory, 8> CATEGORY_ORDER = {
        ItemCategory::Consumables,
        ItemCategory::Materials,
        ItemCategory::KeyItems,
        ItemCategory::Spells,
        ItemCategory::WeaponsShields,
        ItemCategory::ArrowsBolts,
        ItemCategory::Armor,
        ItemCategory::Rings,
    };

    // Suffix of the category icons (:/dsr_images/inventory_<suffix>[_hover]).
    QString categoryIcon(ItemCategory category) {
        switch (category) {
            case ItemCategory::Consumables: return "consumables";
            case ItemCategory::Materials: return "materials";
            case ItemCategory::KeyItems: return "key_items";
            case ItemCategory::ArrowsBolts: return "ammunition";
            case ItemCategory::WeaponsShields: return "weapons_shields";
            case ItemCategory::Rings: return "rings";
            case ItemCategory::Armor: return "armor";
            case ItemCategory::Spells: return "spells";
        }
        return {};
    }

    QString categoryLabel(ItemCategory category) {
        switch (category) {
            case ItemCategory::Consumables: return "Consumables";
            case ItemCategory::Materials: return "Upgrade Materials";
            case ItemCategory::KeyItems: return "Key Items";
            case ItemCategory::ArrowsBolts: return "Ammunition";
            case ItemCategory::WeaponsShields: return "Weapons/Shields";
            case ItemCategory::Rings: return "Rings";
            case ItemCategory::Armor: return "Armor";
            case ItemCategory::Spells: return "Spells";
        }
        return {};
    }

    // Stackable goods show their count, kept in one row for the inventory and the box.
    bool isStackCategory(ItemCategory category) {
        return category == ItemCategory::Consumables || category == ItemCategory::Materials
            || category == ItemCategory::ArrowsBolts;
    }

    QString fromView(std::string_view s) {
        return QString::fromUtf8(s.data(), int(s.size()));
    }

    // Infusion name and icon (:/dsr_inv_images/<icon>); the icon brightens from +5.
    std::pair<QString, QString> infusionInfo(uint16_t infusion, uint8_t level) {
        switch (infusion) {
            case 100: return {"Crystal", "crystal"};
            case 200: return {"Lightning", "lightning"};
            case 300: return {"Raw", "raw"};
            case 400: return {"Magic", level >= 5 ? "magic_2" : "magic"};
            case 500: return {"Enchanted", "enchanted"};
            case 600: return {"Divine", level >= 5 ? "divine_2" : "divine"};
            case 700: return {"Occult", "occult"};
            case 800: return {"Fire", level >= 5 ? "fire_2" : "fire"};
            case 900: return {"Chaos", "chaos"};
            default: return {};
        }
    }

    // The id the equipment slots store: the base id with the infusion and level folded back in.
    uint32_t equipId(const fssm::parse::dsr::InventoryItem& item) {
        if (item.itemId == 1330000 || item.itemId == 1332000) return item.itemId + item.upgradeLevel * 100;
        return item.itemId + item.infusion + item.upgradeLevel;
    }

    // Ids in the slots of one category's equipment, as stored by ChrAsm and the attunement slots.
    std::set<uint32_t> equippedIds(const fssm::parse::dsr::DSRCharacterInfo& c, ItemCategory category) {
        switch (category) {
            case ItemCategory::WeaponsShields: return {c.lHandSlot1, c.lHandSlot2, c.rHandSlot1, c.rHandSlot2};
            case ItemCategory::ArrowsBolts: return {c.lArrowsSlot, c.lBoltsSlot, c.rArrowsSlot, c.rBoltsSlot};
            case ItemCategory::Armor: return {c.head_slot, c.body_slot, c.arms_slot, c.legs_slot};
            case ItemCategory::Rings: return {c.lRingSlot, c.rRingSlot};
            case ItemCategory::Consumables: return {c.q1Slot, c.q2Slot, c.q3Slot, c.q4Slot, c.q5Slot};
            case ItemCategory::Spells: {
                std::set<uint32_t> ids;
                for (const auto& slot : c.attunementSlots) ids.insert(slot.spellId);
                return ids;
            }
            default: return {};
        }
    }
}

InventoryWidget::InventoryWidget(QWidget* parent): QWidget(parent) {
    setAttribute(Qt::WA_TranslucentBackground, true);
    setMouseTracking(true);
}

void InventoryWidget::setCharacter(const fssm::parse::dsr::DSRCharacterInfo* charInfo) {
    m_entries.clear();
    m_hasCharacter = charInfo != nullptr;
    if (charInfo != nullptr) {
        const auto& c = *charInfo;
        std::unordered_map<ItemCategory, std::set<uint32_t>> equipped;
        for (const ItemCategory category : CATEGORY_ORDER) equipped[category] = equippedIds(c, category);
        // Stackable goods held and stored are one row; everything else gets one row per piece.
        std::unordered_map<uint32_t, size_t> stackRows;
        const auto makeEntry = [&](const fssm::parse::dsr::InventoryItem& item) {
            Entry entry;
            entry.category = item.baseItem.category;
            entry.known = item.knownItem;
            entry.order = item.order;
            entry.durability = item.durability;
            entry.maxStack = item.baseItem.max_stack_count;
            if (item.knownItem) {
                // The item DB writes the flasks as "Estus Flask + 7"; the game as "Estus Flask+7".
                static const QRegularExpression spacedLevel(" \\+ (\\d+)");
                entry.name = fromView(item.baseItem.label).replace(spacedLevel, "+\\1");
                if (item.upgradeLevel > 0) entry.name += QString("+%1").arg(item.upgradeLevel);
                entry.image = fromView(item.baseItem.image);
                const auto [infusion, icon] = infusionInfo(item.infusion, item.upgradeLevel);
                entry.infusion = infusion;
                entry.infusionImage = icon;
                const auto& ids = equipped[entry.category];
                entry.equipped = ids.count(equipId(item)) > 0 && equipId(item) != EMPTY_SLOT;
            } else {
                entry.name = QString("Unknown item %1 %2").arg(item.itemType).arg(item.itemId);
            }
            return entry;
        };
        const auto isHidden = [](const fssm::parse::dsr::InventoryItem& item) {
            if (!item.knownItem) return false;
            // Fists and the "no armor" pieces fill empty equipment slots.
            if (item.baseItem.type == 0 && item.baseItem.id == 900000) return true;
            if (item.baseItem.type == 268435456) {
                switch (item.baseItem.id) {
                    case 900000: case 901000: case 902000: case 903000: return true;
                    default: break;
                }
            }
            return false;
        };
        for (const auto& item : c.inventoryItems) {
            if (isHidden(item)) continue;
            Entry entry = makeEntry(item);
            entry.held = true;
            entry.amount = item.amount;
            if (isStackCategory(entry.category)) stackRows[item.itemId] = m_entries.size();
            m_entries.push_back(std::move(entry));
        }
        for (const auto& item : c.bottomlessBoxItems) {
            if (isHidden(item)) continue;
            if (isStackCategory(item.baseItem.category)) {
                if (auto it = stackRows.find(item.itemId); it != stackRows.end()) {
                    m_entries[it->second].stored = item.amount;
                    m_entries[it->second].inBox = true;
                    continue;
                }
            }
            Entry entry = makeEntry(item);
            entry.inBox = true;
            entry.stored = item.amount;
            if (isStackCategory(entry.category)) stackRows[item.itemId] = m_entries.size();
            m_entries.push_back(std::move(entry));
        }
        std::stable_sort(m_entries.begin(), m_entries.end(), [](const Entry& a, const Entry& b) {
            if (a.order != b.order) return a.order < b.order;
            return a.name.compare(b.name, Qt::CaseInsensitive) < 0;
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
        if (entry.category == category) m_visible.push_back(&entry);
    }
    m_selectedItem = m_visible.empty() ? -1 : std::clamp(m_selectedItem, 0, int(m_visible.size()) - 1);
    updateScrollRange();
}

void InventoryWidget::selectCategory(int category) {
    category = std::clamp(category, 0, int(CATEGORY_ORDER.size()) - 1);
    if (category == m_category) return;
    m_category = category;
    m_scroll.reset();
    m_selectedItem = 0;
    m_hoverItem = -1;
    rebuildVisibleItems();
    update();
}

InventoryWidget::Layout InventoryWidget::computeLayout() const {
    Layout l;
    const double s = FIXED_SCALE;
    const auto px = [s](double v) { return int(std::round(v * s)); };
    const QRect r = rect();

    // The detail panel takes the right side, where the game shows the status summary;
    // below the list when that leaves the list too narrow.
    const int detailW = px(400);
    const bool beside = r.width() >= px(1000);
    QRect listArea = r;
    if (beside) {
        l.detail = QRect(r.right() - detailW + 1, r.top(), detailW, r.height());
        listArea.setRight(l.detail.left() - px(20));
    } else {
        const int detailH = px(260);
        l.detail = QRect(r.left(), r.bottom() - detailH + 1, r.width(), detailH);
        listArea.setBottom(l.detail.top() - px(16));
    }

    // Category bar across the list, as many icons as categories.
    const int tabsH = px(112);
    const int count = int(CATEGORY_ORDER.size());
    l.tabWidth = std::min(px(126), (listArea.width() - px(16)) / count);
    const int tabsW = l.tabWidth * count + px(16);
    l.tabs = QRect(listArea.left(), listArea.top(), std::max(tabsW, listArea.width()), tabsH);

    const int listTop = l.tabs.bottom() + 1 + px(18);
    const int trackW = px(10);
    l.scrollTrack = QRect(listArea.left() + px(4), listTop, trackW, listArea.bottom() - listTop + 1);
    l.list = QRect(l.scrollTrack.right() + px(12), listTop, listArea.right() - l.scrollTrack.right() - px(12),
                   listArea.bottom() - listTop + 1);
    return l;
}

void InventoryWidget::updateScrollRange() {
    const int rowH = int(std::round(140 * FIXED_SCALE));
    m_scroll.setGeometry(m_layout.list, m_layout.scrollTrack, int(m_visible.size()) * rowH);
    m_scroll.setStep(rowH);
}

void InventoryWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    m_layout = computeLayout();
    updateScrollRange();
}

QPixmap InventoryWidget::scaled(const QString& resource, const QSize& size) {
    const QString key = resource + QString("@%1x%2").arg(size.width()).arg(size.height());
    auto it = m_scaledCache.find(key);
    if (it != m_scaledCache.end()) return *it;
    QPixmap pix(resource);
    if (!pix.isNull()) pix = pix.scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    m_scaledCache.insert(key, pix);
    return pix;
}

void InventoryWidget::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    painter.setClipRect(event->rect());
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    if (!m_hasCharacter) {
        m_scroll.clear();
        painter.setFont(serifFont(int(std::round(36 * FIXED_SCALE))));
        drawShadowText(painter, rect(), Qt::AlignCenter, "No character selected", DIM_TEXT_COLOR);
        return;
    }
    drawCategories(painter);
    drawList(painter);
    drawDetail(painter);
}

void InventoryWidget::drawCategories(QPainter& painter) {
    const double s = FIXED_SCALE;
    const auto px = [s](double v) { return int(std::round(v * s)); };
    const QRect bar = m_layout.tabs;

    // The game's grey stitched strip behind the icons.
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, false);
    QLinearGradient stone(bar.topLeft(), bar.bottomLeft());
    stone.setColorAt(0.0, QColor(92, 90, 86));
    stone.setColorAt(1.0, QColor(66, 65, 62));
    painter.fillRect(bar, stone);
    painter.drawTiledPixmap(bar, grainTexture());
    painter.setPen(QPen(QColor(20, 19, 18), 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(bar.adjusted(0, 0, -1, -1));
    QPen stitch(QColor(180, 176, 166, 90), 1, Qt::DashLine);
    painter.setPen(stitch);
    painter.drawRect(bar.adjusted(px(6), px(6), -px(6) - 1, -px(6) - 1));
    painter.restore();

    const int count = int(CATEGORY_ORDER.size());
    const int left = bar.left() + (bar.width() - m_layout.tabWidth * count) / 2;
    const QSize iconSize(m_layout.tabWidth - px(6), bar.height() - px(14));
    for (int i = 0; i < count; ++i) {
        const QRect cell(left + i * m_layout.tabWidth, bar.top() + px(7), m_layout.tabWidth, bar.height() - px(14));
        const bool current = i == m_category;
        const QString name = categoryIcon(CATEGORY_ORDER[i]);
        const QPixmap pix = scaled(":/dsr_images/inventory_" + name + (current || i == m_hoverTab ? "_hover" : ""), iconSize);
        if (current) painter.fillRect(cell.adjusted(px(4), 0, -px(4), 0), QColor(30, 28, 26, 150));
        if (!current && i == m_hoverTab) painter.setOpacity(0.75);
        painter.drawPixmap(cell.center().x() - pix.width() / 2, cell.center().y() - pix.height() / 2, pix);
        painter.setOpacity(1.0);
        if (current) {
            // Bronze square around the current category.
            const QRect frame = cell.adjusted(px(4), 0, -px(4), 0);
            painter.setPen(QPen(QColor(150, 110, 52), std::max(1.0, 3 * s)));
            painter.setBrush(Qt::NoBrush);
            painter.drawRect(QRectF(frame).adjusted(0.5, 0.5, -0.5, -0.5));
        }
    }
}

void InventoryWidget::drawItemIcon(QPainter& painter, const Entry& entry, const QRect& rect) {
    const double s = FIXED_SCALE;
    const auto px = [s](double v) { return int(std::round(v * s)); };
    // The item stands on a dish, as everywhere in the game's menus.
    const QPixmap stand = scaled(":/dsr_images/inventory_stand", QSize(rect.width(), rect.width() / 2));
    painter.drawPixmap(rect.left() + (rect.width() - stand.width()) / 2, rect.bottom() - stand.height() + 1, stand);
    QPixmap pix;
    if (!entry.known) {
        pix = scaled(":/dsr_images/unknown", rect.size());
    } else if (inventoryResourcesAvailable() && !entry.image.isEmpty()) {
        pix = scaled(":/dsr_inv_images/" + entry.image, rect.size() - QSize(px(4), px(8)));
    }
    if (!pix.isNull()) {
        painter.drawPixmap(rect.left() + (rect.width() - pix.width()) / 2, rect.top() + px(2), pix);
    }
    if (!entry.infusionImage.isEmpty() && inventoryResourcesAvailable()) {
        const int size = rect.width() * 36 / 100;
        const QPixmap infusion = scaled(":/dsr_inv_images/" + entry.infusionImage, QSize(size, size));
        painter.drawPixmap(rect.right() - infusion.width() + 1, rect.bottom() - infusion.height() - px(8), infusion);
    }
}

void InventoryWidget::drawList(QPainter& painter) {
    const double s = FIXED_SCALE;
    const auto px = [s](double v) { return int(std::round(v * s)); };
    const QRect list = m_layout.list;
    const int rowH = px(140);
    const int iconColW = px(132);

    painter.save();
    painter.setClipRect(list.adjusted(0, -px(6), 0, px(6)), Qt::IntersectClip);
    if (m_visible.empty()) {
        painter.setFont(serifFont(px(30)));
        drawShadowText(painter, list.adjusted(0, px(20), 0, 0), Qt::AlignHCenter | Qt::AlignTop, "No items", DIM_TEXT_COLOR);
    }
    const int first = std::max(0, m_scroll.value() / rowH);
    const int last = std::min(int(m_visible.size()) - 1, (m_scroll.value() + list.height()) / rowH);
    for (int i = first; i <= last; ++i) {
        const Entry& entry = *m_visible[i];
        const QRect row(list.left(), list.top() + i * rowH - m_scroll.value(), list.width(), rowH);
        const QRect iconCol(row.left(), row.top() + px(4), iconColW, rowH - px(8));
        const QRect plate(iconCol.right() + 1, row.top() + px(4), row.right() - iconCol.right(), rowH - px(8));
        const bool selected = i == m_selectedItem;

        // Item on its dish in a darker cell; the name on a plate, burnt orange when selected.
        fillLeather(painter, iconCol, selected ? QColor(92, 64, 44, 200) : QColor(0, 0, 0, 60));
        if (selected) {
            fillLeather(painter, plate, SELECTED_COLOR);
        } else if (i == m_hoverItem) {
            painter.fillRect(plate, QColor(SELECTED_COLOR.red(), SELECTED_COLOR.green(), SELECTED_COLOR.blue(), 60));
        }
        painter.setPen(QPen(QColor(FRAME_COLOR.red(), FRAME_COLOR.green(), FRAME_COLOR.blue(), 150), 1));
        painter.drawLine(iconCol.right(), iconCol.top(), iconCol.right(), iconCol.bottom());

        const int iconSize = iconColW - px(26);
        drawItemIcon(painter, entry, QRect(iconCol.center().x() - iconSize / 2, iconCol.top() + px(8), iconSize, iconSize));
        if (entry.equipped) drawEquippedSeal(painter, QPointF(iconCol.left() + px(16), iconCol.top() + px(14)), px(13));
        // Count at the bottom right of the icon, on a short rule, for goods that stack.
        const bool showCount = isStackCategory(entry.category)
            || (entry.category == ItemCategory::KeyItems && entry.amount > 1);
        if (showCount && entry.held) {
            painter.setFont(serifFont(px(30)));
            const QRect countRect(iconCol.left(), iconCol.bottom() - px(40), iconCol.width() - px(8), px(36));
            drawShadowText(painter, countRect, Qt::AlignRight | Qt::AlignBottom, QString::number(entry.amount));
            painter.setPen(QPen(QColor(220, 214, 200, 200), 1));
            painter.drawLine(iconCol.right() - px(60), iconCol.bottom() - px(3), iconCol.right() - px(8), iconCol.bottom() - px(3));
        }

        // Name, and below it where the item is kept, with the Bottomless Box count.
        const QRect text = plate.adjusted(px(30), px(8), -px(16), -px(8));
        painter.setFont(serifFont(px(32)));
        drawShadowText(painter, QRect(text.left(), text.top(), text.width(), text.height() / 2),
                       Qt::AlignLeft | Qt::AlignVCenter, entry.name);
        painter.setFont(serifFont(px(24)));
        const QRect sub(text.left(), text.top() + text.height() / 2, text.width(), text.height() / 2);
        const int keptIcon = px(30);
        int x = sub.left();
        const auto kept = [&](const QString& icon, const QString& value) {
            painter.drawPixmap(x, sub.center().y() - keptIcon / 2, scaled(icon, QSize(keptIcon, keptIcon)));
            x += keptIcon + px(6);
            if (!value.isEmpty()) {
                const int w = QFontMetrics(painter.font()).horizontalAdvance(value);
                drawShadowText(painter, QRect(x, sub.top(), w + 2, sub.height()), Qt::AlignLeft | Qt::AlignVCenter, value);
                x += w;
            }
            x += px(26);
        };
        const bool stack = isStackCategory(entry.category);
        if (entry.held) kept(":/dsr_images/inventory_bag", stack ? QString::number(entry.amount) : QString());
        if (entry.inBox) kept(":/dsr_images/bottomless_box", stack ? QString::number(entry.stored) : QString());
        if (!entry.infusion.isEmpty()) {
            drawShadowText(painter, QRect(x, sub.top(), sub.right() - x, sub.height()), Qt::AlignLeft | Qt::AlignVCenter,
                           entry.infusion, AREA_COLOR);
        }
        if (i + 1 < int(m_visible.size())) drawKnotRule(painter, row.left(), row.right(), row.bottom() + 1, s * 1.6);
    }
    painter.restore();

    // Thin scroll bar left of the rows, as in the game.
    m_scroll.paint(painter, QColor(0, 0, 0, 120), SELECTED_COLOR.lighter(120));
}

void InventoryWidget::drawDetail(QPainter& painter) {
    const double s = FIXED_SCALE;
    const auto px = [s](double v) { return int(std::round(v * s)); };
    const QRect panel = m_layout.detail;
    drawPanelFrame(painter, panel, s * 1.4);
    const QRect inner = panel.adjusted(px(28), px(28), -px(28), -px(24));
    const Entry* entry = m_selectedItem >= 0 && m_selectedItem < int(m_visible.size()) ? m_visible[m_selectedItem] : nullptr;

    // Like the game's item description: the icon in a framed box beside the name, then
    // text sitting on ruled lines down the panel.
    const int iconBox = px(132);
    const QRect iconRect(inner.left(), inner.top(), iconBox, iconBox);
    painter.setPen(QPen(QColor(FRAME_COLOR.red(), FRAME_COLOR.green(), FRAME_COLOR.blue(), 170), 1));
    painter.setBrush(QColor(0, 0, 0, 50));
    painter.drawRect(QRectF(iconRect).adjusted(0.5, 0.5, -0.5, -0.5));
    if (entry != nullptr) drawItemIcon(painter, *entry, iconRect.adjusted(px(10), px(8), -px(10), -px(6)));

    // Below the list the panel is wide: the ruled lines go on the right half.
    const bool wide = panel.width() > px(700);
    const int nameRight = wide ? inner.left() + inner.width() / 2 - px(24) : inner.right();
    const QRect nameRect(iconRect.right() + px(16), inner.top(), nameRight - iconRect.right() - px(16), iconBox);
    painter.setFont(serifFont(px(30)));
    if (entry != nullptr) {
        // Wrapped at spaces only: Qt would break "Estus Flask+7" before the "+".
        const QFontMetrics fm(painter.font());
        QStringList lines;
        for (const QString& word : entry->name.split(' ')) {
            if (!lines.isEmpty() && fm.horizontalAdvance(lines.last() + ' ' + word) <= nameRect.width()) {
                lines.last() += ' ' + word;
            } else {
                lines.append(word);
            }
        }
        const int lineH = fm.height();
        const int maxLines = std::max(1, (nameRect.height() - px(6)) / lineH);
        for (int i = 0; i < std::min(int(lines.size()), maxLines); ++i) {
            QString text = lines[i];
            if (i == maxLines - 1 && lines.size() > maxLines) text += QString::fromUtf8("…");
            drawShadowText(painter, QRect(nameRect.left(), nameRect.top() + px(6) + i * lineH, nameRect.width(), lineH),
                           Qt::AlignLeft | Qt::AlignVCenter, text);
        }
    }

    std::vector<std::pair<QString, QString>> lines;
    if (entry != nullptr) {
        lines.push_back({"Type", categoryLabel(entry->category)});
        if (!entry->infusion.isEmpty()) lines.push_back({"Infusion", entry->infusion});
        if (entry->category == ItemCategory::WeaponsShields || entry->category == ItemCategory::Armor) {
            lines.push_back({"Durability", QString::number(entry->durability)});
        }
        if (isStackCategory(entry->category)) {
            lines.push_back({"Held", entry->maxStack > 1 ? QString("%1/%2").arg(entry->amount).arg(entry->maxStack)
                                                         : QString::number(entry->amount)});
            lines.push_back({"Bottomless Box", QString::number(entry->stored)});
        } else {
            lines.push_back({"Kept in", entry->held ? "Inventory" : "Bottomless Box"});
        }
        if (entry->equipped) lines.push_back({"Equipped", ""});
    }

    const int lineH = px(52);
    painter.setFont(serifFont(px(26)));
    const int linesLeft = wide ? nameRight + px(24) : inner.left();
    const int linesTop = wide ? inner.top() : iconRect.bottom() + px(12);
    const QRect lineArea(linesLeft, linesTop, inner.right() - linesLeft + 1, inner.bottom() - linesTop + 1);
    for (int row = 0; lineArea.top() + (row + 1) * lineH <= lineArea.bottom() + 1; ++row) {
        const QRect line(lineArea.left(), lineArea.top() + row * lineH, lineArea.width(), lineH);
        if (row < int(lines.size())) {
            // The label keeps its width; a long value is cut short instead.
            const QRect text = line.adjusted(px(4), 0, -px(4), -px(4));
            const int labelW = QFontMetrics(painter.font()).horizontalAdvance(lines[row].first);
            drawShadowText(painter, text, Qt::AlignLeft | Qt::AlignBottom, lines[row].first);
            drawShadowText(painter, text.adjusted(labelW + px(16), 0, 0, 0), Qt::AlignRight | Qt::AlignBottom,
                           lines[row].second);
        }
        drawRule(painter, line.left(), line.right(), line.bottom(), 150);
    }
}

int InventoryWidget::itemIndexAt(const QPoint& pos) const {
    if (!m_layout.list.contains(pos)) return -1;
    const int rowH = int(std::round(140 * FIXED_SCALE));
    const int idx = (pos.y() - m_layout.list.top() + m_scroll.value()) / rowH;
    return idx < int(m_visible.size()) ? idx : -1;
}

int InventoryWidget::tabIndexAt(const QPoint& pos) const {
    const QRect bar = m_layout.tabs;
    if (!bar.contains(pos) || m_layout.tabWidth <= 0) return -1;
    const int count = int(CATEGORY_ORDER.size());
    const int left = bar.left() + (bar.width() - m_layout.tabWidth * count) / 2;
    if (pos.x() < left) return -1;
    const int idx = (pos.x() - left) / m_layout.tabWidth;
    return idx < count ? idx : -1;
}

void InventoryWidget::mouseMoveEvent(QMouseEvent* event) {
    if (m_scroll.mouseMove(event)) return;
    const int hover = m_hasCharacter ? itemIndexAt(event->pos()) : -1;
    const int tab = m_hasCharacter ? tabIndexAt(event->pos()) : -1;
    if (hover != m_hoverItem || tab != m_hoverTab) {
        m_hoverItem = hover;
        m_hoverTab = tab;
        setCursor(tab >= 0 && tab != m_category ? Qt::PointingHandCursor : Qt::ArrowCursor);
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
    if (const int tab = tabIndexAt(event->pos()); tab >= 0) {
        selectCategory(tab);
    } else if (const int item = itemIndexAt(event->pos()); item >= 0) {
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
    if (m_layout.tabs.contains(pos)) {
        selectCategory(m_category - event->angleDelta().y() / 120);
    } else if (m_layout.list.adjusted(-m_layout.list.left() + m_layout.scrollTrack.left(), 0, 0, 0).contains(pos)) {
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
