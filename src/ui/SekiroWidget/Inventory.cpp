#include "Inventory.h"

#include <QImage>
#include <QRadialGradient>
#include <QMouseEvent>
#include <QPainter>
#include <QWheelEvent>
#include <algorithm>
#include <array>
#include <initializer_list>

#include "../Utils.h"

namespace fssm::ui::sekiro {
using fssm::parse::sekiro::InventoryItem;
using fssm::parse::sekiro::ItemCategory;
using fssm::parse::sekiro::SkillNode;

namespace {
    struct Category {
        const char* label;
        const char* icon;           // resource path
        std::initializer_list<ItemCategory> categories;     // empty on the Skills tab
        bool tile = false;          // icon needs the dark tile drawn under it
    };

    struct Page {
        const char* label;
        std::initializer_list<Category> categories;
    };

    // The game's inventory tabs, then its Acquire Skills screen: one category per
    // skill tree, in SkillNode::tree order, shown with the tree's Esoteric Text.
    const std::array<Page, 2> PAGES = {{
        {"Inventory", {
            {"Quick Items", ":/sekiro_images/category_quick_items", {ItemCategory::QuickItems}},
            {"Key Items", ":/sekiro_images/category_key_items",
             {ItemCategory::KeyItems, ItemCategory::PrayerBeads, ItemCategory::Memories, ItemCategory::SpiritEmblems}},
            {"Upgrade Materials", ":/sekiro_images/category_materials", {ItemCategory::Materials}},
            {"Prosthetic Tools", ":/sekiro_images/category_prosthetics", {ItemCategory::ProstheticTools}},
        }},
        {"Skills", {
            {"Shinobi Arts", ":/sekiro_inv_images/910", {}, true},
            {"Prosthetic Arts", ":/sekiro_inv_images/930", {}, true},
            {"Ashina Arts", ":/sekiro_inv_images/900", {}, true},
            {"Temple Arts", ":/sekiro_inv_images/920", {}, true},
            {"Mushin Arts", ":/sekiro_inv_images/940", {}, true},
        }},
    }};
    constexpr int SKILLS_PAGE = 1;
    constexpr int SKILL_ROWS = 8;
    constexpr int SKILL_COLUMNS = 4;

    // Size the layout is designed for; everything scales from it.
    constexpr int DESIGN_W = 600;
    constexpr int DESIGN_H = 430;

    const Category& categoryAt(int page, int category) {
        return *(PAGES[page].categories.begin() + category);
    }

    int categoryCount(int page) {
        return int(PAGES[page].categories.size());
    }

    bool showsCount(const InventoryItem& item) {
        const ItemCategory c = item.baseItem.category;
        if (c == ItemCategory::ProstheticTools || c == ItemCategory::CombatArts) return false;
        return item.quantity > 1 || item.baseItem.maxNum > 1;
    }

    QString label(std::string_view text) {
        return QString::fromUtf8(text.data(), int(text.size()));
    }

    fssm::parse::sekiro::BaseItem skillItem(const SkillNode& node) {
        return fssm::parse::sekiro::findBaseItem(node.itemId).value_or(fssm::parse::sekiro::BaseItem{});
    }

    // Ornate octagon the game puts around some nodes: the secret techniques that
    // open the Mushin Arts (also shown at the start of that tree) and the Mushin Arts.
    enum class SkillFrame { None, Ougi, Hiden };

    SkillFrame skillFrame(const SkillNode& node) {
        constexpr int MUSHIN_TREE = 4;
        if (node.tree == MUSHIN_TREE && node.column > 0) return SkillFrame::Hiden;
        for (const auto& other : fssm::parse::sekiro::SKILL_TREE) {
            if (other.tree == MUSHIN_TREE && other.column == 0 && other.itemId == node.itemId) return SkillFrame::Ougi;
        }
        return SkillFrame::None;
    }

    // Colour of the icon's painted ring, used for the glow of learned skills.
    QColor ringColor(const QPixmap& icon) {
        const QImage image = icon.toImage();
        if (image.isNull()) return ORANGE_COLOR;
        const QColor c = image.pixelColor(image.width() / 10, image.height() / 2);
        return c.alpha() < 64 ? ORANGE_COLOR : c;
    }
}

InventoryWidget::InventoryWidget(QWidget* parent): QWidget(parent) {
    setAttribute(Qt::WA_TranslucentBackground, true);
    setMouseTracking(true);
}

QSize InventoryWidget::sizeHint() const {
    return {DESIGN_W, DESIGN_H};
}

void InventoryWidget::setCharacter(const fssm::parse::sekiro::SekiroCharacterInfo* charInfo) {
    m_hasCharacter = charInfo != nullptr;
    m_stats = charInfo != nullptr ? *charInfo : fssm::parse::sekiro::SekiroCharacterInfo{};
    m_allItems = std::move(m_stats.inventoryItems);
    m_stats.inventoryItems.clear();
    m_heldItems.clear();
    for (const auto& item : m_allItems) m_heldItems.insert(item.itemId);
    std::stable_sort(m_allItems.begin(), m_allItems.end(), [](const InventoryItem& a, const InventoryItem& b) {
        if (a.baseItem.order != b.baseItem.order) return a.baseItem.order < b.baseItem.order;
        return a.itemId < b.itemId;
    });
    selectPage(m_page, m_category);
}

void InventoryWidget::selectPage(int page, int category) {
    m_page = std::clamp(page, 0, int(PAGES.size()) - 1);
    m_category = std::clamp(category, 0, categoryCount(m_page) - 1);
    m_scroll = 0;
    m_hoverItem = -1;
    m_layout = computeLayout();
    rebuildVisibleItems();
    m_selectedItem = m_visibleItems.empty() && m_treeNodes.empty() ? -1 : 0;
    update();
}

void InventoryWidget::rebuildVisibleItems() {
    m_visibleItems.clear();
    m_treeNodes.clear();
    if (skillPage()) {
        for (const auto& node : fssm::parse::sekiro::SKILL_TREE) {
            if (node.tree == m_category) m_treeNodes.push_back(&node);
        }
        std::stable_sort(m_treeNodes.begin(), m_treeNodes.end(), [](const SkillNode* a, const SkillNode* b) {
            return std::make_pair(a->column, a->row) < std::make_pair(b->column, b->row);
        });
        m_scroll = 0;
        return;
    }
    const Category& category = categoryAt(m_page, m_category);
    for (const auto& item : m_allItems) {
        const auto& wanted = category.categories;
        if (std::find(wanted.begin(), wanted.end(), item.baseItem.category) != wanted.end()) {
            m_visibleItems.push_back(&item);
        }
    }
    m_scroll = std::clamp(m_scroll, 0, maxScroll());
}

InventoryWidget::Layout InventoryWidget::computeLayout() const {
    Layout l;
    const QRect r = rect();
    l.scale = std::clamp(std::min(double(r.width()) / DESIGN_W, double(r.height()) / DESIGN_H), 0.7, 1.8);
    const double s = l.scale;
    auto px = [s](double v) { return int(std::round(v * s)); };

    const int tabW = px(130);
    for (int i = 0; i < int(PAGES.size()); ++i) {
        l.tabs.emplace_back(r.left() + px(16) + i * (tabW + px(10)), r.top(), tabW, px(34));
    }
    // The Acquire Skills screen has no status bar; the tree gets the room.
    l.status = m_page == SKILLS_PAGE ? QRect(r.left(), r.bottom() + 1, r.width(), 0)
                                     : QRect(r.left(), r.bottom() + 1 - px(56), r.width(), px(56));

    const int top = r.top() + px(44);
    const int leftW = int(r.width() * 0.52);
    l.categoryLabel = QRect(r.left() + px(14), top, leftW - px(28), px(22));
    int listTop = l.categoryLabel.bottom() + 1 + px(6);
    if (categoryCount(m_page) > 1) {
        const int size = px(40);
        for (int i = 0; i < categoryCount(m_page); ++i) {
            l.categories.emplace_back(r.left() + px(26) + i * (size + px(12)), l.categoryLabel.bottom() + 1 + px(4),
                                      size, size);
        }
        listTop = l.categories.back().bottom() + 1 + px(10);
    }
    const int bottom = l.status.top() - (l.status.height() > 0 ? px(10) : px(2));
    l.list = QRect(r.left(), listTop, leftW - px(16), std::max(0, bottom - listTop));
    l.rowHeight = px(46);
    l.scrollTrack = QRect(l.list.right() + px(6), l.list.top() + px(4), std::max(2, px(3)),
                          std::max(0, l.list.height() - px(8)));
    l.detail = QRect(r.left() + leftW + px(4), top - px(4), r.width() - leftW - px(4), bottom - top + px(4));
    return l;
}

int InventoryWidget::maxScroll() const {
    if (m_layout.rowHeight <= 0) return 0;
    return std::max(0, int(m_visibleItems.size()) * m_layout.rowHeight - m_layout.list.height());
}

void InventoryWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    m_layout = computeLayout();
    m_cache.clear();
    m_scroll = std::clamp(m_scroll, 0, maxScroll());
}

bool InventoryWidget::skillPage() const {
    return m_page == SKILLS_PAGE;
}

bool InventoryWidget::learned(const SkillNode& node) const {
    return m_heldItems.count(node.itemId) > 0;
}

QPoint InventoryWidget::nodeCenter(const SkillNode& node, int* nodeSize) const {
    const QRect canvas = m_layout.list;
    const double margin = 14 * m_layout.scale;
    // Proportions of the game's screen: rows ~97 px apart, nodes ~90 px, columns ~276 px.
    const double rowStep = canvas.height() / double(SKILL_ROWS);
    const int size = int(rowStep * 0.9);
    const double colStep = std::min(rowStep * 2.85, (canvas.width() - size - 2 * margin) / double(SKILL_COLUMNS - 1));
    if (nodeSize != nullptr) *nodeSize = size;
    return {int(canvas.left() + margin + size / 2.0 + node.column * colStep),
            int(canvas.top() + rowStep * (node.row + 0.5))};
}

QPixmap InventoryWidget::skillIcon(const SkillNode& node, int size) {
    const auto item = skillItem(node);
    if (!inventoryResourcesAvailable() || item.image.empty()) return {};
    return m_cache.fit(":/sekiro_inv_images/" + label(item.image), QSize(size, size));
}

// The skill's icon is what the game shows for a skill not acquired yet. Acquired
// skills get the Skill screen's treatment on top: a glow in the ring's colour, a
// brightened centre and the red 修 ("mastered") stamp.
void InventoryWidget::drawSkillNode(QPainter& painter, const SkillNode& node, const QRect& box) {
    const bool isLearned = learned(node);
    const SkillFrame frame = skillFrame(node);
    const QPointF c = QRectF(box).center();
    const int size = box.width();
    const int iconSize = frame == SkillFrame::None ? size : int(size * 0.8);
    const QPixmap icon = skillIcon(node, iconSize);

    if (isLearned) {
        QColor glow = ringColor(icon).lighter(150);
        QRadialGradient halo(c, size * 0.72);
        glow.setAlpha(235);
        halo.setColorAt(0.5, glow);
        glow.setAlpha(0);
        halo.setColorAt(1.0, glow);
        painter.setPen(Qt::NoPen);
        painter.setBrush(halo);
        painter.drawEllipse(c, size * 0.72, size * 0.72);
    }
    if (!icon.isNull()) {
        painter.drawPixmap(int(c.x() - icon.width() / 2.0), int(c.y() - icon.height() / 2.0), icon);
    }
    if (isLearned && !icon.isNull()) {
        // Lift the parchment towards white while the ink stays dark.
        painter.save();
        painter.setCompositionMode(QPainter::CompositionMode_SoftLight);
        QRadialGradient bloom(c, iconSize * 0.42);
        bloom.setColorAt(0.0, QColor(255, 255, 255, 255));
        bloom.setColorAt(1.0, QColor(255, 255, 255, 0));
        painter.setPen(Qt::NoPen);
        painter.setBrush(bloom);
        painter.drawEllipse(c, iconSize * 0.42, iconSize * 0.42);
        painter.drawEllipse(c, iconSize * 0.42, iconSize * 0.42);
        painter.restore();
    }
    if (frame != SkillFrame::None) {
        const int frameSize = int(size * 1.08);
        const QString resource = frame == SkillFrame::Ougi ? ":/sekiro_images/skill_frame_ougi"
                                                           : ":/sekiro_images/skill_frame_hiden";
        painter.drawPixmap(int(c.x() - frameSize / 2.0), int(c.y() - frameSize / 2.0),
                           m_cache.get(resource, QSize(frameSize, frameSize)));
    }
    if (isLearned) {
        const int stamp = std::max(8, int(size * 0.26));
        const QRect mark(box.right() - stamp + int(size * 0.06), box.top() - int(size * 0.04), stamp, stamp);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(196, 36, 24, 235));
        painter.drawRoundedRect(mark, stamp * 0.12, stamp * 0.12);
        QFont font;
        font.setFamilies({"SimSun", "MingLiU", "Yu Gothic"});
        font.setPixelSize(std::max(6, int(stamp * 0.8)));
        painter.setFont(font);
        painter.setPen(QColor(255, 236, 220));
        painter.drawText(mark, Qt::AlignCenter, QString(QChar(0x4FEE)));
    }
}

QPixmap InventoryWidget::itemIcon(const InventoryItem& item, int size) {
    if (!inventoryResourcesAvailable() || item.baseItem.image.empty()) return {};
    return m_cache.fit(":/sekiro_inv_images/" + label(item.baseItem.image), QSize(size, size));
}

void InventoryWidget::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    painter.setClipRect(event->rect());
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);
    if (m_layout.rowHeight == 0) m_layout = computeLayout();

    drawTabs(painter);
    if (!m_hasCharacter) {
        painter.setFont(serifFont(int(20 * m_layout.scale)));
        drawShadowText(painter, rect(), Qt::AlignCenter, "No character selected", DIM_TEXT_COLOR);
        return;
    }
    drawCategories(painter);
    if (skillPage()) {
        drawSkillTree(painter);
    } else {
        drawList(painter);
        drawStatus(painter);
    }
    drawDetail(painter);
}

void InventoryWidget::drawTabs(QPainter& painter) {
    const double s = m_layout.scale;
    painter.setFont(serifFont(int(19 * s)));
    for (int i = 0; i < int(m_layout.tabs.size()); ++i) {
        const QRect tab = m_layout.tabs[i];
        if (i == m_page) {
            const QRect stroke = tab.adjusted(-int(10 * s), -int(4 * s), int(10 * s), int(6 * s));
            painter.drawPixmap(stroke.topLeft(), m_cache.get(":/sekiro_images/tab_selected", stroke.size()));
        } else {
            painter.drawPixmap(tab.topLeft(), m_cache.get(":/sekiro_images/tab", tab.size()));
        }
        drawShadowText(painter, tab, Qt::AlignCenter, PAGES[i].label);
    }
    // Rule under the tabs.
    const int y = m_layout.tabs.front().bottom() + int(5 * s);
    painter.setPen(QPen(QColor(30, 28, 24, 110), std::max(1.0, 1.5 * s)));
    painter.drawLine(QPoint(rect().left() + int(8 * s), y), QPoint(rect().right() - int(8 * s), y));
}

void InventoryWidget::drawCategories(QPainter& painter) {
    const Layout& l = m_layout;
    const double s = l.scale;
    const QRect labelBar = l.categoryLabel.adjusted(0, 0, -l.categoryLabel.width() / 3, 0);
    painter.setOpacity(0.55);
    painter.drawPixmap(labelBar.topLeft(), m_cache.get(":/sekiro_images/header_bar", labelBar.size()));
    painter.setOpacity(1.0);
    painter.setFont(serifFont(int(16 * s)));
    QString count = QString::number(m_visibleItems.size());
    if (skillPage()) {
        const auto learnedCount = std::count_if(m_treeNodes.begin(), m_treeNodes.end(),
                                                [this](const SkillNode* n) { return learned(*n); });
        count = QString("%1/%2").arg(learnedCount).arg(m_treeNodes.size());
    }
    drawShadowText(painter, l.categoryLabel.adjusted(int(14 * s), 0, 0, 0), Qt::AlignLeft | Qt::AlignVCenter,
                   QString("%1  (%2)").arg(categoryAt(m_page, m_category).label, count));

    for (int i = 0; i < int(l.categories.size()); ++i) {
        const QRect cell = l.categories[i];
        if (i == m_category) {
            const QRect glow = cell.adjusted(-int(5 * s), -int(5 * s), int(5 * s), int(5 * s));
            painter.drawPixmap(glow.topLeft(), m_cache.get(":/sekiro_images/category_cursor", glow.size()));
        }
        const Category& category = categoryAt(m_page, i);
        if (category.tile) {
            painter.fillRect(cell, QColor(16, 15, 13, 235));
            painter.setPen(QPen(QColor(120, 112, 96), 1));
            painter.setBrush(Qt::NoBrush);
            painter.drawRect(cell.adjusted(0, 0, -1, -1));
        }
        painter.setOpacity(i == m_category ? 1.0 : 0.85);
        const QRect iconRect = category.tile ? cell.adjusted(int(2 * s), int(2 * s), -int(2 * s), -int(2 * s)) : cell;
        painter.drawPixmap(iconRect.topLeft(), m_cache.get(category.icon, iconRect.size()));
        painter.setOpacity(1.0);
    }
}

void InventoryWidget::drawList(QPainter& painter) {
    const Layout& l = m_layout;
    const double s = l.scale;
    painter.save();
    painter.setClipRect(l.list, Qt::IntersectClip);

    const int rowH = l.rowHeight;
    const int box = rowH - int(4 * s);
    const int firstRow = m_scroll / rowH;
    const int lastRow = std::min(int(m_visibleItems.size()) - 1, (m_scroll + l.list.height()) / rowH);
    const QFont nameFont = serifFont(int(17 * s));
    const QFont countFont = serifFont(int(14 * s));
    for (int row = firstRow; row <= lastRow; ++row) {
        const InventoryItem& item = *m_visibleItems[row];
        const QRect cell(l.list.left(), l.list.top() + row * rowH - m_scroll, l.list.width(), rowH);
        const QRect iconBox(cell.left() + int(18 * s), cell.top() + (rowH - box) / 2, box, box);
        painter.drawPixmap(iconBox.topLeft(), m_cache.get(":/sekiro_images/icon_base", iconBox.size()));
        const QPixmap icon = itemIcon(item, box - int(4 * s));
        if (!icon.isNull()) {
            painter.drawPixmap(iconBox.center().x() - icon.width() / 2 + 1, iconBox.center().y() - icon.height() / 2 + 1, icon);
        }
        if (showsCount(item)) {
            painter.setFont(countFont);
            drawShadowText(painter, iconBox.adjusted(0, 0, -int(1 * s), int(1 * s)), Qt::AlignRight | Qt::AlignBottom,
                           QString::number(item.quantity));
        }

        const int strokeH = int(rowH * 0.82);
        const QRect stroke(iconBox.right() + int(8 * s), cell.top() + (rowH - strokeH) / 2,
                           cell.right() - iconBox.right() - int(8 * s), strokeH);
        painter.drawPixmap(stroke.topLeft(), m_cache.get(":/sekiro_images/item_row", stroke.size()));
        const bool selected = row == m_selectedItem;
        if (selected || row == m_hoverItem) {
            painter.setOpacity(selected ? 1.0 : 0.35);
            const QRect cursor = stroke.adjusted(-int(4 * s), 0, 0, 0);
            painter.drawPixmap(cursor.topLeft(), m_cache.get(":/sekiro_images/item_row_selected", cursor.size()));
            painter.setOpacity(1.0);
        }
        painter.setFont(nameFont);
        drawShadowText(painter, stroke.adjusted(int(22 * s), 0, -int(28 * s), 0), Qt::AlignLeft | Qt::AlignVCenter,
                       label(item.baseItem.label));
    }
    painter.restore();

    const int total = maxScroll();
    if (total > 0) {
        const QRect track = l.scrollTrack;
        painter.fillRect(track, QColor(30, 28, 24, 120));
        const int thumbH = std::max(int(18 * s), track.height() * track.height() / (track.height() + total));
        const int thumbY = track.top() + (track.height() - thumbH) * m_scroll / total;
        painter.fillRect(QRect(track.left() - 1, thumbY, track.width() + 2, thumbH), QColor(236, 230, 214));
    }
}

void InventoryWidget::drawDetail(QPainter& painter) {
    const Layout& l = m_layout;
    const double s = l.scale;
    painter.drawPixmap(l.detail.topLeft(), m_cache.get(":/sekiro_images/detail_panel", l.detail.size()));
    const QRect inner = l.detail.adjusted(int(14 * s), int(12 * s), -int(14 * s), -int(14 * s));
    if (skillPage()) {
        drawSkillDetail(painter, inner);
        return;
    }
    if (m_selectedItem < 0 || m_selectedItem >= int(m_visibleItems.size())) {
        painter.setFont(serifFont(int(16 * s)));
        drawShadowText(painter, inner, Qt::AlignCenter, "No items", DIM_TEXT_COLOR);
        return;
    }
    const InventoryItem& item = *m_visibleItems[m_selectedItem];

    const QRect title(inner.left(), inner.top(), inner.width(), int(32 * s));
    painter.drawPixmap(title.topLeft(), m_cache.get(":/sekiro_images/title_bar", title.size()));
    painter.setFont(serifFont(int(18 * s)));
    drawShadowText(painter, title.adjusted(int(24 * s), 0, -int(24 * s), 0), Qt::AlignCenter, label(item.baseItem.label));

    // The stat rows (inventory page only) need room left of the icon.
    const QFont statFont = serifFont(int(14 * s));
    const QFontMetrics statMetrics(statFont);
    const int statsNeeded = statMetrics.horizontalAdvance("Number Held") + statMetrics.horizontalAdvance("000/ 000")
                            + int(40 * s);
    const bool showsHeld = m_page == 0 && (item.baseItem.maxNum > 1 || item.quantity > 1);
    const int iconSize = showsHeld
        ? std::clamp(inner.width() - statsNeeded - int(12 * s), int(56 * s), int(112 * s))
        : std::min(int(112 * s), inner.width() * 34 / 100);
    const QRect iconBox(inner.right() - iconSize - int(6 * s), title.bottom() + int(10 * s), iconSize, iconSize);
    painter.drawPixmap(iconBox.topLeft(), m_cache.get(":/sekiro_images/icon_base", iconBox.size()));
    const QPixmap icon = itemIcon(item, iconSize - int(6 * s));
    if (!icon.isNull()) {
        painter.drawPixmap(iconBox.center().x() - icon.width() / 2 + 1, iconBox.center().y() - icon.height() / 2 + 1, icon);
    }

    int textTop = iconBox.bottom() + int(10 * s);
    if (showsHeld) {
        const int rowH = int(26 * s);
        const int statsW = iconBox.left() - inner.left() - int(8 * s);
        // Rows sit next to the icon, lower when it is big enough (like the game).
        const int statsTop = iconBox.top() + std::max(0, (iconSize - 2 * rowH - int(8 * s)) / 2);
        auto statRow = [&](int index, const QString& name, const QString& value) {
            const QRect bar(inner.left(), statsTop + index * (rowH + int(8 * s)), statsW, rowH);
            textTop = std::max(textTop, bar.bottom() + int(10 * s));
            painter.drawPixmap(bar.topLeft(), m_cache.get(":/sekiro_images/stat_bar", bar.size()));
            painter.setFont(statFont);
            const QRect text = bar.adjusted(int(10 * s), 0, -int(16 * s), 0);
            drawShadowText(painter, text, Qt::AlignLeft | Qt::AlignVCenter, name);
            drawShadowText(painter, text, Qt::AlignRight | Qt::AlignVCenter, value);
        };
        const uint16_t maxNum = item.baseItem.maxNum;
        const ItemCategory c = item.baseItem.category;
        const bool limited = maxNum > 1 && (c == ItemCategory::QuickItems || c == ItemCategory::Materials);
        statRow(0, "Number Held", limited ? QString("%1/ %2").arg(item.quantity).arg(maxNum) : QString::number(item.quantity));
        statRow(1, "In Storage", QString::number(item.storageQuantity));
    }

    // Description on ruled lines.
    const QFont descFont = serifFont(int(14 * s));
    const int lineH = QFontMetrics(descFont).lineSpacing();
    // Whole lines only; text that does not fit is cut at a line break.
    const int descH = (inner.bottom() - textTop) / lineH * lineH;
    const QRect desc(inner.left() + int(6 * s), textTop, inner.width() - int(12 * s), descH);
    if (desc.height() <= 0) return;
    painter.save();
    painter.setClipRect(desc.adjusted(-2, 0, 2, 2), Qt::IntersectClip);
    painter.setFont(descFont);
    painter.setPen(QPen(QColor(240, 236, 226, 45), 1));
    for (int y = desc.top() + lineH; y < desc.bottom(); y += lineH) {
        painter.drawLine(QPoint(desc.left(), y + 1), QPoint(desc.right(), y + 1));
    }
    drawShadowText(painter, desc, Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap, label(item.baseItem.description));
    painter.restore();
}

void InventoryWidget::drawStatus(QPainter& painter) {
    const Layout& l = m_layout;
    const double s = l.scale;
    const int gap = int(8 * s);
    const int plateW = (l.status.width() - 2 * gap) / 3;
    const QRect hpPlate(l.status.left(), l.status.top(), plateW, l.status.height());
    const QRect statsPlate(hpPlate.right() + 1 + gap, l.status.top(), plateW, l.status.height());
    const QRect senPlate(statsPlate.right() + 1 + gap, l.status.top(), l.status.right() - statsPlate.right() - gap,
                         l.status.height());
    for (const QRect& plate : {hpPlate, statsPlate, senPlate}) {
        painter.setOpacity(0.92);
        painter.drawPixmap(plate.topLeft(), m_cache.get(":/sekiro_images/plate", plate.size()));
        painter.setOpacity(1.0);
    }
    const QFont font = serifFont(int(15 * s));
    painter.setFont(font);

    // Vitality (HP) and Posture gauges.
    auto gauge = [&](int line, const char* name, uint32_t value, uint32_t maxValue, const QColor& color) {
        const int rowH = (hpPlate.height() - int(10 * s)) / 2;
        const QRect half(hpPlate.left() + int(12 * s), hpPlate.top() + int(5 * s) + line * rowH,
                         hpPlate.width() - int(24 * s), rowH);
        const int labelW = QFontMetrics(font).horizontalAdvance("Posture") + int(10 * s);
        drawShadowText(painter, QRect(half.left(), half.top(), labelW, half.height()), Qt::AlignLeft | Qt::AlignVCenter, name);
        const QString numbers = QString("%1/%2").arg(value).arg(maxValue);
        const int numbersW = QFontMetrics(font).horizontalAdvance(numbers) + int(4 * s);
        drawShadowText(painter, half, Qt::AlignRight | Qt::AlignVCenter, numbers);
        const QRect bar(half.left() + labelW, half.center().y() - int(3 * s), half.width() - labelW - numbersW - int(6 * s),
                        std::max(3, int(6 * s)));
        if (bar.width() <= 0) return;
        painter.fillRect(bar, QColor(20, 18, 16, 220));
        const double ratio = maxValue > 0 ? std::clamp(double(value) / maxValue, 0.0, 1.0) : 0.0;
        painter.fillRect(QRect(bar.left(), bar.top(), int(bar.width() * ratio), bar.height()), color);
    };
    gauge(0, "HP", m_stats.hp, m_stats.maxHp, QColor(214, 90, 70));
    gauge(1, "Posture", m_stats.posture, m_stats.maxPosture, QColor(232, 160, 50));

    // Vitality / Attack Power with the game's status icons.
    auto statLine = [&](int line, const char* icon, const char* name, uint32_t value) {
        const int rowH = (statsPlate.height() - int(10 * s)) / 2;
        const QRect row(statsPlate.left() + int(10 * s), statsPlate.top() + int(5 * s) + line * rowH,
                        statsPlate.width() - int(22 * s), rowH);
        const int iconSize = rowH - int(2 * s);
        painter.drawPixmap(row.left(), row.top() + (rowH - iconSize) / 2,
                           m_cache.fit(QString(":/sekiro_images/") + icon, QSize(iconSize, iconSize)));
        const QRect text = row.adjusted(iconSize + int(6 * s), 0, 0, 0);
        drawShadowText(painter, text, Qt::AlignLeft | Qt::AlignVCenter, name);
        drawShadowText(painter, text, Qt::AlignRight | Qt::AlignVCenter, QString::number(value));
    };
    statLine(0, "icon_vitality", "Vitality", m_stats.vitality);
    statLine(1, "icon_attack", "Attack Power", m_stats.attackPower);

    // Sen.
    const int senIcon = std::min(senPlate.height() - int(14 * s), int(34 * s));
    const QRect senRow = senPlate.adjusted(int(14 * s), 0, -int(16 * s), 0);
    painter.drawPixmap(senRow.left(), senRow.center().y() - senIcon / 2,
                       m_cache.fit(":/sekiro_images/icon_sen", QSize(senIcon, senIcon)));
    painter.setFont(serifFont(int(20 * s)));
    drawShadowText(painter, senRow.adjusted(senIcon + int(8 * s), 0, 0, 0), Qt::AlignRight | Qt::AlignVCenter,
                   QString::number(m_stats.sen));
}

int InventoryWidget::itemIndexAt(const QPoint& pos) const {
    const Layout& l = m_layout;
    if (skillPage()) {
        for (int i = 0; i < int(m_treeNodes.size()); ++i) {
            int size = 0;
            const QPoint d = pos - nodeCenter(*m_treeNodes[i], &size);
            if (d.x() * d.x() + d.y() * d.y() <= size * size / 4) return i;
        }
        return -1;
    }
    if (!l.list.contains(pos) || l.rowHeight <= 0) return -1;
    const int idx = (pos.y() - l.list.top() + m_scroll) / l.rowHeight;
    return idx < int(m_visibleItems.size()) ? idx : -1;
}

void InventoryWidget::mouseMoveEvent(QMouseEvent* event) {
    const int hover = m_hasCharacter ? itemIndexAt(event->pos()) : -1;
    if (hover != m_hoverItem) {
        m_hoverItem = hover;
        update();
    }
    QString tip;
    for (int i = 0; i < int(m_layout.categories.size()); ++i) {
        if (m_layout.categories[i].contains(event->pos())) tip = categoryAt(m_page, i).label;
    }
    if (skillPage() && m_hoverItem >= 0) tip = label(skillItem(*m_treeNodes[m_hoverItem]).label);
    setToolTip(tip);
    QWidget::mouseMoveEvent(event);
}

void InventoryWidget::mousePressEvent(QMouseEvent* event) {
    if (!m_hasCharacter || event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }
    const QPoint pos = event->pos();
    for (int i = 0; i < int(m_layout.tabs.size()); ++i) {
        if (m_layout.tabs[i].contains(pos) && i != m_page) {
            selectPage(i, 0);
            return;
        }
    }
    for (int i = 0; i < int(m_layout.categories.size()); ++i) {
        if (m_layout.categories[i].contains(pos) && i != m_category) {
            selectPage(m_page, i);
            return;
        }
    }
    if (const int item = itemIndexAt(pos); item >= 0) {
        m_selectedItem = item;
        update();
    }
}

void InventoryWidget::wheelEvent(QWheelEvent* event) {
    if (!m_hasCharacter || skillPage()) return;
    const int steps = event->angleDelta().y() / 120;
    m_scroll = std::clamp(m_scroll - steps * m_layout.rowHeight, 0, maxScroll());
    m_hoverItem = itemIndexAt(event->position().toPoint());
    update();
    event->accept();
}

void InventoryWidget::leaveEvent(QEvent* event) {
    m_hoverItem = -1;
    update();
    QWidget::leaveEvent(event);
}

void InventoryWidget::drawSkillTree(QPainter& painter) {
    const double s = m_layout.scale;
    // Connectors first: right from the prerequisite, along to halfway, then into the skill.
    painter.setPen(QPen(QColor(28, 26, 22, 220), std::max(1.5, 2.2 * s), Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin));
    painter.setBrush(Qt::NoBrush);
    for (const SkillNode* node : m_treeNodes) {
        int size = 0;
        const QPoint to = nodeCenter(*node, &size);
        for (const int32_t pre : node->prerequisites) {
            const auto parent = std::find_if(m_treeNodes.begin(), m_treeNodes.end(),
                                             [pre](const SkillNode* n) { return n->rowId == pre; });
            if (pre < 0 || parent == m_treeNodes.end()) continue;
            const QPoint from = nodeCenter(**parent);
            const int midX = (from.x() + size / 2 + to.x() - size / 2) / 2;
            const QPoint points[4] = {from, QPoint(midX, from.y()), QPoint(midX, to.y()), to};
            painter.drawPolyline(points, 4);
            painter.setBrush(QColor(28, 26, 22));
            const int dot = std::max(2, int(3 * s));
            painter.drawEllipse(QPoint(midX, to.y()), dot, dot);
            painter.setBrush(Qt::NoBrush);
        }
    }

    for (int i = 0; i < int(m_treeNodes.size()); ++i) {
        const SkillNode& node = *m_treeNodes[i];
        int size = 0;
        const QPoint c = nodeCenter(node, &size);
        const QRect box(c.x() - size / 2, c.y() - size / 2, size, size);
        drawSkillNode(painter, node, box);
        if (i == m_selectedItem || i == m_hoverItem) {
            painter.setBrush(Qt::NoBrush);
            painter.setPen(QPen(i == m_selectedItem ? ORANGE_COLOR : QColor(240, 236, 226, 160), std::max(2.0, 3 * s)));
            painter.drawEllipse(box.adjusted(-int(3 * s), -int(3 * s), int(3 * s), int(3 * s)));
        }
    }
}

void InventoryWidget::drawSkillDetail(QPainter& painter, const QRect& inner) {
    const double s = m_layout.scale;
    if (m_selectedItem < 0 || m_selectedItem >= int(m_treeNodes.size())) return;
    const SkillNode& node = *m_treeNodes[m_selectedItem];
    const auto item = skillItem(node);
    const bool isLearned = learned(node);

    const QRect title(inner.left(), inner.top(), inner.width(), int(32 * s));
    painter.drawPixmap(title.topLeft(), m_cache.get(":/sekiro_images/title_bar", title.size()));
    painter.setFont(serifFont(int(18 * s)));
    drawShadowText(painter, title.adjusted(int(24 * s), 0, -int(24 * s), 0), Qt::AlignCenter, label(item.label));

    painter.setFont(serifFont(int(14 * s)));
    const QRect state(inner.left() + int(6 * s), title.bottom() + int(2 * s), inner.width(), int(20 * s));
    drawShadowText(painter, state, Qt::AlignLeft | Qt::AlignVCenter, isLearned ? "Learned" : "Not learned",
                   isLearned ? TEXT_COLOR : DIM_TEXT_COLOR);

    // The game's ink illustration of the skill, or its icon when there is none.
    int textTop = state.bottom() + int(6 * s);
    QPixmap story;
    if (inventoryResourcesAvailable() && !item.image.empty()) {
        const int w = inner.width() - int(8 * s);
        story = m_cache.fit(":/sekiro_skill_images/" + label(item.image), QSize(w, w));
    }
    painter.setOpacity(isLearned ? 1.0 : 0.55);
    if (!story.isNull()) {
        painter.drawPixmap(inner.center().x() - story.width() / 2, textTop, story);
        textTop += story.height() + int(8 * s);
    } else {
        const int size = std::min(int(96 * s), inner.width() / 3);
        const QPixmap icon = skillIcon(node, size);
        if (!icon.isNull()) painter.drawPixmap(inner.center().x() - icon.width() / 2, textTop, icon);
        textTop += size + int(8 * s);
    }
    painter.setOpacity(1.0);

    const QFont descFont = serifFont(int(14 * s));
    const int lineH = QFontMetrics(descFont).lineSpacing();
    const int descH = (inner.bottom() - textTop) / lineH * lineH;
    const QRect desc(inner.left() + int(6 * s), textTop, inner.width() - int(12 * s), descH);
    if (desc.height() <= 0) return;
    painter.save();
    painter.setClipRect(desc.adjusted(-2, 0, 2, 2), Qt::IntersectClip);
    painter.setFont(descFont);
    painter.setPen(QPen(QColor(240, 236, 226, 45), 1));
    for (int y = desc.top() + lineH; y < desc.bottom(); y += lineH) {
        painter.drawLine(QPoint(desc.left(), y + 1), QPoint(desc.right(), y + 1));
    }
    drawShadowText(painter, desc, Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap, label(item.description));
    painter.restore();
}
}
