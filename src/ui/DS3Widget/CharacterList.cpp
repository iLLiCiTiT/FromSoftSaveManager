#include "CharacterList.h"

#include <QAbstractItemView>
#include <QPaintEvent>
#include <QPainter>
#include <cmath>

#include "Painting.h"

namespace fssm::ui::ds3 {
namespace {
    constexpr int ROW_HEIGHT = 76;
    constexpr int TITLE_HEIGHT = 32;

    // Game sprites (tools/ds3_flags/gen_menu.py). The slot is 88 px tall with gold rules
    // under its two text rows; it stretches only between its ornamented ends.
    constexpr int SLOT_HEIGHT = 88;
    constexpr int SLOT_LEFT = 30;
    constexpr int SLOT_RIGHT = 24;
    constexpr int SLOT_TOP_ROW_BOTTOM = 31;     // first rule at y 32
    constexpr int SLOT_BOTTOM_ROW_TOP = 36;
    constexpr int SLOT_BOTTOM_ROW_BOTTOM = 62;  // second rule at y 63
    // The panel's frame and inner bevel, kept unscaled by the nine-slice.
    constexpr int PANEL_BORDER = 56;
    constexpr double PANEL_SCALE = 0.3;

    QString formatPlayTime(uint32_t seconds) {
        return QString("%1:%2:%3").arg(seconds / 3600, 2, 10, QChar('0'))
                                  .arg((seconds / 60) % 60, 2, 10, QChar('0'))
                                  .arg(seconds % 60, 2, 10, QChar('0'));
    }

    const QPixmap& slotPixmap() {
        static const QPixmap pix(":/ds3_images/load_slot");
        return pix;
    }

    const QPixmap& slotCursorPixmap() {
        static const QPixmap pix(":/ds3_images/load_slot_cursor");
        return pix;
    }
}

CharacterDelegate::CharacterDelegate(QObject* parent): QStyledItemDelegate(parent) {}

QSize CharacterDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex&) const {
    // Rows take the viewport's width (see paint); only the height matters here.
    return {1, ROW_HEIGHT};
}

void CharacterDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    // Rows span the visible list, whatever width the view laid them out with.
    QRect r = option.rect;
    if (const auto* view = qobject_cast<const QAbstractItemView*>(option.widget)) {
        r.setWidth(view->viewport()->width() - r.left());
    }
    const QVariant name = index.data(CharNameRole);
    if (!name.isValid()) {
        // Error message from the model.
        painter->setFont(serifFont(13));
        drawShadowText(*painter, r.adjusted(10, 4, -10, -4), Qt::AlignLeft | Qt::AlignVCenter | Qt::TextWordWrap,
                       index.data(Qt::DisplayRole).toString(), DIM_TEXT_COLOR);
        painter->restore();
        return;
    }
    const bool selected = option.state & QStyle::State_Selected;
    const bool hovered = option.state & QStyle::State_MouseOver;
    const QRect slot = r.adjusted(0, 2, 0, -2);
    const double s = double(slot.height()) / SLOT_HEIGHT;
    painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
    drawNineSlice(*painter, slotPixmap(), slot, SLOT_LEFT, 0, SLOT_RIGHT, 0, s);
    if (selected || hovered) {
        // The game's orange glow over the selected slot; fainter on hover.
        painter->setOpacity(selected ? 1.0 : 0.4);
        drawNineSlice(*painter, slotCursorPixmap(), slot, SLOT_LEFT, 0, SLOT_RIGHT, 0, s);
        painter->setOpacity(1.0);
    }

    // Text sits on the slot's gold rules: name and area above the first, level and
    // play time above the second.
    const int left = slot.left() + int(std::round((SLOT_LEFT + 4) * s));
    const int right = slot.right() - int(std::round((SLOT_RIGHT + 2) * s));
    const QRect top(left, slot.top(), right - left, int(std::round(SLOT_TOP_ROW_BOTTOM * s)));
    const QRect bottom(left, slot.top() + int(std::round(SLOT_BOTTOM_ROW_TOP * s)), right - left,
                       int(std::round((SLOT_BOTTOM_ROW_BOTTOM - SLOT_BOTTOM_ROW_TOP) * s)));
    const int leftWidth = top.width() * 42 / 100;
    const int rightLeft = left + top.width() * 46 / 100;

    painter->setFont(serifFont(16));
    // The area takes whatever width the name leaves, so long area names fit beside short names.
    const int nameWidth = std::min(QFontMetrics(painter->font()).horizontalAdvance(name.toString()), right - left);
    drawShadowText(*painter, QRect(top.left(), top.top(), nameWidth, top.height()),
                   Qt::AlignLeft | Qt::AlignBottom, name.toString());
    drawShadowText(*painter, QRect(bottom.left(), bottom.top(), leftWidth, bottom.height()),
                   Qt::AlignLeft | Qt::AlignBottom, "Level");
    drawShadowText(*painter, QRect(bottom.left(), bottom.top(), leftWidth - 6, bottom.height()),
                   Qt::AlignRight | Qt::AlignBottom, QString::number(index.data(CharLevelRole).toUInt()));
    painter->setFont(serifFont(15));
    const int areaLeft = top.left() + nameWidth + 10;
    drawShadowText(*painter, QRect(areaLeft, top.top(), right - areaLeft, top.height()),
                   Qt::AlignRight | Qt::AlignBottom, index.data(CharAreaRole).toString());
    painter->setFont(serifFont(16));
    drawShadowText(*painter, QRect(rightLeft, bottom.top(), right - rightLeft, bottom.height()),
                   Qt::AlignRight | Qt::AlignBottom, formatPlayTime(index.data(CharPlayTimeRole).toUInt()));
    painter->restore();
}

LoadGameFrame::LoadGameFrame(QWidget* content, QWidget* parent): QWidget(parent), m_content(content) {
    setAttribute(Qt::WA_TranslucentBackground, true);
    m_icon = QPixmap(":/ds3_images/menu_status");
    m_panel = QPixmap(":/ds3_images/load_panel");
    m_content->setParent(this);
    setMinimumWidth(300);
}

QSize LoadGameFrame::sizeHint() const {
    return {350, 400};
}

QRect LoadGameFrame::wellRect() const {
    const int border = int(std::round(PANEL_BORDER * PANEL_SCALE));
    return rect().adjusted(border, TITLE_HEIGHT + border, -border, -border);
}

void LoadGameFrame::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    m_content->setGeometry(wellRect().adjusted(4, 6, -4, -6));
}

void LoadGameFrame::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    painter.setClipRect(event->rect());
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // Title with the menu icon, like "LOAD GAME" above the slots in the game.
    const int iconSize = 26;
    if (!m_icon.isNull()) {
        painter.drawPixmap(QRect(4, (TITLE_HEIGHT - iconSize) / 2, iconSize, iconSize), m_icon);
    }
    painter.setFont(serifFont(18));
    drawShadowText(painter, QRect(iconSize + 11, 0, width() - iconSize - 13, TITLE_HEIGHT),
                   Qt::AlignLeft | Qt::AlignVCenter, "Characters");

    // The game's framed panel the slots sit in.
    drawNineSlice(painter, m_panel, rect().adjusted(0, TITLE_HEIGHT, 0, 0),
                  PANEL_BORDER, PANEL_BORDER, PANEL_BORDER, PANEL_BORDER, PANEL_SCALE);
}
}
