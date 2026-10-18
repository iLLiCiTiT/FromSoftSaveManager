#include "CharacterList.h"

#include <QAbstractItemView>
#include <QImage>
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QRandomGenerator>

namespace fssm::ui::dsr {
namespace {
    constexpr int ROW_HEIGHT = 58;
    constexpr int TITLE_HEIGHT = 30;
    constexpr int PANEL_PADDING = 8;
    // Width of the carved gold frame around a slot.
    constexpr int FRAME = 5;

    const QColor TEXT_COLOR(232, 226, 212);
    const QColor DIM_TEXT_COLOR(150, 144, 132);
    // The game's gold, toned down from an HDR capture.
    const QColor GOLD_COLOR(158, 128, 76);
    const QColor GOLD_LIGHT_COLOR(196, 166, 108);
    const QColor GOLD_DARK_COLOR(74, 56, 30);
    const QColor SELECTED_COLOR(138, 62, 24);

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
        painter.save();
        painter.setPen(QColor(0, 0, 0, 210));
        painter.drawText(rect.translated(1, 1), flags, elided);
        painter.setPen(color);
        painter.drawText(rect, flags, elided);
        painter.restore();
    }

    // Tileable grain that gives flat fills the look of the game's worn stone.
    const QPixmap& grainTexture() {
        static const QPixmap texture = [] {
            QImage image(128, 128, QImage::Format_ARGB32_Premultiplied);
            QRandomGenerator rng(0xd5a);
            for (int y = 0; y < image.height(); ++y) {
                auto* line = reinterpret_cast<QRgb*>(image.scanLine(y));
                for (int x = 0; x < image.width(); ++x) {
                    const int alpha = int(rng.bounded(30));
                    const int v = rng.bounded(2) ? 255 : 0;
                    line[x] = qPremultiply(qRgba(v, v, v, alpha));
                }
            }
            return QPixmap::fromImage(image);
        }();
        return texture;
    }

    QString formatPlayTime(uint32_t seconds) {
        return QString("%1:%2:%3").arg(seconds / 3600)
                                  .arg((seconds / 60) % 60, 2, 10, QChar('0'))
                                  .arg(seconds % 60, 2, 10, QChar('0'));
    }

    // Slot: carved gold frame (light top edge, darker bottom edge, a groove in the
    // middle) around a see-through plate, as in the game; the selected plate is burnt orange.
    void drawSlot(QPainter& painter, const QRect& r, bool selected, bool hovered) {
        painter.save();
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
            painter.setBrush(SELECTED_COLOR);
            painter.drawRect(plate);
            painter.drawTiledPixmap(plate, grainTexture());
        } else if (hovered) {
            painter.setBrush(QColor(SELECTED_COLOR.red(), SELECTED_COLOR.green(), SELECTED_COLOR.blue(), 60));
            painter.drawRect(plate);
        }
        // Inner shadow where the plate sits under the frame.
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(QColor(0, 0, 0, 150), 1));
        painter.drawRect(plate.adjusted(0, 0, -1, -1));
        painter.setPen(QPen(GOLD_LIGHT_COLOR, 1));
        painter.drawLine(plate.left(), plate.bottom() + 1, plate.right(), plate.bottom() + 1);
        painter.restore();
    }
}

CharacterDelegate::CharacterDelegate(QObject* parent): QStyledItemDelegate(parent) {}

QSize CharacterDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex&) const {
    // Rows take the viewport's width (see paint); only the height matters here.
    return {1, ROW_HEIGHT};
}

void CharacterDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    painter->save();
    // Rows span the visible list, whatever width the view laid them out with.
    QRect r = option.rect;
    if (const auto* view = qobject_cast<const QAbstractItemView*>(option.widget)) {
        r.setWidth(view->viewport()->width() - r.left());
    }
    const QVariant name = index.data(CharNameRole);
    if (!name.isValid()) {
        // Error message from the model.
        painter->setFont(serifFont(13));
        drawText(*painter, r.adjusted(10, 4, -10, -4), Qt::AlignLeft | Qt::AlignVCenter | Qt::TextWordWrap,
                 index.data(Qt::DisplayRole).toString(), DIM_TEXT_COLOR);
        painter->restore();
        return;
    }
    const bool selected = option.state & QStyle::State_Selected;
    const bool hovered = option.state & QStyle::State_MouseOver;
    const QRect slot = r.adjusted(0, 2, 0, -2);
    drawSlot(*painter, slot, selected, hovered);

    const QRect text = slot.adjusted(FRAME + 8, FRAME + 2, -FRAME - 8, -FRAME - 2);
    const int half = text.height() / 2;
    const QRect top(text.left(), text.top(), text.width(), half);
    const QRect bottom(text.left(), text.top() + half, text.width(), text.height() - half);
    // Name and level on the left, area and play time on the right, as in the game.
    const int rightLeft = text.left() + text.width() * 38 / 100;
    const int valueRight = rightLeft - 10;

    painter->setFont(serifFont(14));
    drawText(*painter, QRect(top.left(), top.top(), valueRight - top.left(), top.height()),
             Qt::AlignLeft | Qt::AlignVCenter, name.toString());
    drawText(*painter, QRect(rightLeft, top.top(), text.right() - rightLeft, top.height()),
             Qt::AlignLeft | Qt::AlignVCenter, index.data(CharAreaRole).toString());
    painter->setFont(serifFont(13));
    drawText(*painter, QRect(bottom.left(), bottom.top(), valueRight - bottom.left(), bottom.height()),
             Qt::AlignLeft | Qt::AlignVCenter, "Level");
    drawText(*painter, QRect(bottom.left(), bottom.top(), valueRight - bottom.left(), bottom.height()),
             Qt::AlignRight | Qt::AlignVCenter, QString::number(index.data(CharLevelRole).toUInt()));
    drawText(*painter, QRect(rightLeft, bottom.top(), text.right() - rightLeft, bottom.height()),
             Qt::AlignLeft | Qt::AlignVCenter, "Play Time");
    drawText(*painter, QRect(rightLeft, bottom.top(), text.right() - rightLeft, bottom.height()),
             Qt::AlignRight | Qt::AlignVCenter, formatPlayTime(index.data(CharPlayTimeRole).toUInt()));
    painter->restore();
}

DataListFrame::DataListFrame(QWidget* content, QWidget* parent): QWidget(parent), m_content(content) {
    setAttribute(Qt::WA_TranslucentBackground, true);
    m_content->setParent(this);
    setMinimumWidth(240);
}

QSize DataListFrame::sizeHint() const {
    return {270, 400};
}

QRect DataListFrame::panelRect() const {
    return rect().adjusted(0, TITLE_HEIGHT, 0, 0);
}

void DataListFrame::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    m_content->setGeometry(panelRect().adjusted(PANEL_PADDING, PANEL_PADDING - 2, -PANEL_PADDING, -PANEL_PADDING + 2));
}

void DataListFrame::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    painter.setClipRect(event->rect());

    // Title with a gold rule under it, where the game has "Data List".
    painter.setFont(serifFont(18));
    drawText(painter, QRect(2, 0, width() - 4, TITLE_HEIGHT - 4), Qt::AlignLeft | Qt::AlignVCenter, "Characters");
    QLinearGradient rule(0, 0, width(), 0);
    rule.setColorAt(0.0, GOLD_COLOR);
    rule.setColorAt(1.0, QColor(GOLD_COLOR.red(), GOLD_COLOR.green(), GOLD_COLOR.blue(), 0));
    painter.fillRect(QRect(0, TITLE_HEIGHT - 5, width(), 1), rule);
}
}
