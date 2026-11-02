#include "CharacterList.h"

#include <QAbstractItemView>
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>

#include "Painting.h"
#include "../../parse/DSR/WorldProgress.h"

namespace fssm::ui::dsr {
namespace {
    constexpr int ROW_HEIGHT = 60;
    // Same height as the page heading of the header tabs beside the list.
    constexpr int HEADING_HEIGHT = 48;
    constexpr int HEADING_GAP = 8;
    constexpr int PANEL_PADDING = 8;
    // Width of the carved gold frame around a slot.
    constexpr int FRAME = 5;
    // Text inset from the frame and the space between the slot's two columns.
    constexpr int TEXT_INSET = 8;
    constexpr int GAP = 10;
    // Width of the list's scrollbar (app.qss, #ds_list_scrollbar).
    constexpr int SCROLLBAR_WIDTH = 12;
    constexpr int NAME_FONT_SIZE = 15;
    constexpr int VALUE_FONT_SIZE = 14;

    QString formatPlayTime(uint32_t seconds) {
        return QString("%1:%2:%3").arg(seconds / 3600)
                                  .arg((seconds / 60) % 60, 2, 10, QChar('0'))
                                  .arg(seconds % 60, 2, 10, QChar('0'));
    }

    // Slot: carved gold frame (light top edge, darker bottom edge, a groove in the
    // middle) around a see-through plate; the selected plate is burnt orange.
    void drawSlot(QPainter& painter, const QRect& r, bool selected, bool hovered) {
        painter.save();
        painter.setRenderHint(QPainter::Antialiasing, false);
        const QRect plate = r.adjusted(FRAME, FRAME, -FRAME, -FRAME);
        QPainterPath ring;
        ring.setFillRule(Qt::OddEvenFill);
        ring.addRect(r);
        ring.addRect(plate);
        painter.save();
        painter.setClipPath(ring);
        painter.setPen(Qt::NoPen);
        QLinearGradient gold(r.topLeft(), r.bottomLeft());
        gold.setColorAt(0.0, GOLD_LIGHT_COLOR);
        gold.setColorAt(0.5, GOLD_COLOR);
        gold.setColorAt(1.0, GOLD_COLOR.darker(125));
        painter.setBrush(gold);
        painter.drawRect(r);
        painter.drawTiledPixmap(r, grainTexture());
        painter.restore();

        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(GOLD_DARK_COLOR, 1));
        painter.drawRect(r.adjusted(0, 0, -1, -1));
        // Groove along the middle of the frame.
        painter.setPen(QPen(QColor(GOLD_DARK_COLOR.red(), GOLD_DARK_COLOR.green(), GOLD_DARK_COLOR.blue(), 170), 1));
        painter.drawRect(r.adjusted(FRAME / 2, FRAME / 2, -FRAME / 2 - 1, -FRAME / 2 - 1));
        // Carved end pieces: short vertical ridges at both ends of the frame.
        for (const int x : {r.left() + FRAME + 3, r.right() - FRAME - 3}) {
            painter.setPen(QPen(GOLD_DARK_COLOR, 1));
            painter.drawLine(x, r.top() + FRAME, x, r.bottom() - FRAME);
        }

        painter.setPen(Qt::NoPen);
        if (selected) {
            drawSelectedPlate(painter, plate);
        } else if (hovered) {
            painter.fillRect(plate, QColor(SELECTED_COLOR.red(), SELECTED_COLOR.green(), SELECTED_COLOR.blue(), 60));
        }
        // Inner shadow where the plate sits under the frame.
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(QColor(0, 0, 0, 150), 1));
        painter.drawRect(plate.adjusted(0, 0, -1, -1));
        painter.setPen(QPen(GOLD_LIGHT_COLOR, 1));
        painter.drawLine(plate.left(), plate.bottom() + 1, plate.right(), plate.bottom() + 1);
        painter.restore();
    }

    int maxAreaWidth(const QFontMetrics& fm) {
        int width = 0;
        for (const auto& place : fssm::parse::dsr::PLACE_NAMES) {
            width = std::max(width, fm.horizontalAdvance(QString::fromUtf8(place.name.data(), int(place.name.size()))));
        }
        return width;
    }

    // Left column of a slot: wide enough for "Level 999".
    int leftColumnWidth() {
        const QFontMetrics fm(serifFont(VALUE_FONT_SIZE));
        return fm.horizontalAdvance("Level") + 14 + fm.horizontalAdvance("999");
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
    drawSlot(*painter, slot, selected, hovered);

    // As on the game's Data List: name above "Level N" on the left, the area above
    // the play time on the right.
    const int inset = FRAME + TEXT_INSET;
    const QRect text = slot.adjusted(inset, FRAME + 2, -inset, -FRAME - 2);
    const int half = text.height() / 2;
    const int leftW = leftColumnWidth();
    const int rightLeft = text.left() + leftW + GAP;
    const QRect top(text.left(), text.top(), text.width(), half);
    const QRect bottom(text.left(), text.top() + half, text.width(), text.height() - half);

    painter->setFont(serifFont(VALUE_FONT_SIZE));
    const QFontMetrics valueFm(painter->font());
    const QString area = index.data(CharAreaRole).toString();
    // Name and area share the top row: the name keeps up to half of it, the area takes
    // the rest; a short one leaves its space to the other.
    const int nameFullW = QFontMetrics(serifFont(NAME_FONT_SIZE)).horizontalAdvance(name.toString());
    const int nameW = std::min(nameFullW, std::max(text.width() - GAP - valueFm.horizontalAdvance(area), text.width() / 2));
    const int areaW = std::min(valueFm.horizontalAdvance(area), text.width() - GAP - nameW);
    drawShadowText(*painter, QRect(text.right() - areaW + 1, top.top(), areaW, top.height()),
                   Qt::AlignLeft | Qt::AlignVCenter, area);
    drawShadowText(*painter, QRect(bottom.left(), bottom.top(), leftW, bottom.height()),
                   Qt::AlignLeft | Qt::AlignVCenter, "Level");
    drawShadowText(*painter, QRect(bottom.left(), bottom.top(), leftW, bottom.height()),
                   Qt::AlignRight | Qt::AlignVCenter, QString::number(index.data(CharLevelRole).toUInt()));
    const QString playTime = formatPlayTime(index.data(CharPlayTimeRole).toUInt());
    const QRect timeRect(rightLeft, bottom.top(), text.right() - rightLeft + 1, bottom.height());
    drawShadowText(*painter, timeRect, Qt::AlignRight | Qt::AlignVCenter, playTime);
    if (valueFm.horizontalAdvance("Play Time") + GAP + valueFm.horizontalAdvance(playTime) <= timeRect.width()) {
        drawShadowText(*painter, timeRect, Qt::AlignLeft | Qt::AlignVCenter, "Play Time");
    }

    painter->setFont(serifFont(NAME_FONT_SIZE));
    drawShadowText(*painter, QRect(top.left(), top.top(), nameW + 1, top.height()),
                   Qt::AlignLeft | Qt::AlignVCenter, name.toString());
    painter->restore();
}

DataListFrame::DataListFrame(QWidget* content, QWidget* parent): QWidget(parent), m_content(content) {
    setAttribute(Qt::WA_TranslucentBackground, true);
    m_icon = QPixmap(":/dsr_images/menu_data_list");
    m_content->setParent(this);
    setMinimumWidth(sizeHint().width());
}

QSize DataListFrame::sizeHint() const {
    // Wide enough for "Level 999" beside the longest area name, so no slot is cut off.
    const int textWidth = leftColumnWidth() + GAP + maxAreaWidth(QFontMetrics(serifFont(VALUE_FONT_SIZE)));
    return {textWidth + 2 * (FRAME + TEXT_INSET) + SCROLLBAR_WIDTH + 2 * PANEL_PADDING + 2, 400};
}

QRect DataListFrame::panelRect() const {
    return rect().adjusted(0, HEADING_HEIGHT + HEADING_GAP, 0, 0);
}

void DataListFrame::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    m_content->setGeometry(panelRect().adjusted(PANEL_PADDING, PANEL_PADDING, -PANEL_PADDING, -PANEL_PADDING));
}

void DataListFrame::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    painter.setClipRect(event->rect());
    painter.setRenderHint(QPainter::Antialiasing, true);

    // Heading where the game has "Data List / Select data to load".
    drawMenuHeading(painter, QRect(0, 0, width(), HEADING_HEIGHT), m_icon, "Characters", "Select a character",
                    width() - 1);
}
}
