#include "CharacterList.h"

#include <QAbstractItemView>
#include <QPaintEvent>
#include <QPainter>

#include "Painting.h"

namespace fssm::ui::ds3 {
namespace {
    constexpr int ROW_HEIGHT = 66;
    constexpr int TITLE_HEIGHT = 32;
    constexpr int STONE_BORDER = 9;

    QString formatPlayTime(uint32_t seconds) {
        return QString("%1:%2:%3").arg(seconds / 3600, 2, 10, QChar('0'))
                                  .arg((seconds / 60) % 60, 2, 10, QChar('0'))
                                  .arg(seconds % 60, 2, 10, QChar('0'));
    }

    // Slate plate of a save slot: purple-grey stone, dark outline and a light
    // engraved band just inside the edge.
    void drawSlate(QPainter& painter, const QRect& r, bool selected, bool hovered) {
        painter.save();
        QLinearGradient gradient(r.topLeft(), r.bottomLeft());
        gradient.setColorAt(0.0, QColor(116, 112, 126));
        gradient.setColorAt(1.0, QColor(86, 82, 96));
        painter.setPen(Qt::NoPen);
        painter.setBrush(gradient);
        painter.drawRect(r);
        painter.drawTiledPixmap(r, grainTexture());
        if (selected || hovered) {
            QLinearGradient warm(r.topLeft(), r.bottomLeft());
            const int alpha = selected ? 190 : 80;
            warm.setColorAt(0.0, QColor(150, 112, 78, alpha));
            warm.setColorAt(0.55, QColor(170, 118, 66, alpha));
            warm.setColorAt(1.0, QColor(120, 78, 40, alpha));
            painter.setBrush(warm);
            painter.drawRect(r);
        }
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(QColor(20, 18, 22), 1));
        painter.drawRect(r.adjusted(0, 0, -1, -1));
        painter.setPen(QPen(QColor(196, 192, 200, 150), 1, Qt::DotLine));
        painter.drawRect(r.adjusted(3, 3, -4, -4));
        painter.setPen(QPen(QColor(210, 206, 214, 110), 1));
        painter.drawRect(r.adjusted(5, 5, -6, -6));
        if (selected) {
            painter.setPen(QPen(QColor(214, 170, 96, 200), 1));
            painter.drawRect(r.adjusted(1, 1, -2, -2));
        }
        painter.restore();
    }
}

CharacterDelegate::CharacterDelegate(QObject* parent): QStyledItemDelegate(parent) {}

QSize CharacterDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex&) const {
    return {option.rect.width(), ROW_HEIGHT};
}

void CharacterDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    // Rows can be laid out wider than the visible list; keep them inside the viewport.
    QRect r = option.rect;
    if (const auto* view = qobject_cast<const QAbstractItemView*>(option.widget)) {
        r.setWidth(std::min(r.width(), view->viewport()->width() - r.left()));
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
    const QRect plate = r.adjusted(2, 3, -2, -3);
    drawSlate(*painter, plate, selected, hovered);

    const QRect text = plate.adjusted(14, 7, -14, -8);
    const int half = text.height() / 2;
    const QRect top(text.left(), text.top(), text.width(), half);
    const QRect bottom(text.left(), text.top() + half, text.width(), text.height() - half);
    const int leftWidth = text.width() * 42 / 100;
    const int rightLeft = text.left() + text.width() * 46 / 100;

    painter->setFont(serifFont(16));
    drawShadowText(*painter, QRect(top.left(), top.top(), leftWidth, top.height()),
                   Qt::AlignLeft | Qt::AlignVCenter, name.toString());
    drawShadowText(*painter, QRect(bottom.left(), bottom.top(), leftWidth, bottom.height()),
                   Qt::AlignLeft | Qt::AlignVCenter, "Level");
    drawShadowText(*painter, QRect(bottom.left(), bottom.top(), leftWidth - 6, bottom.height()),
                   Qt::AlignRight | Qt::AlignVCenter, QString::number(index.data(CharLevelRole).toUInt()));
    painter->setFont(serifFont(15));
    drawShadowText(*painter, QRect(rightLeft, top.top(), text.right() - rightLeft, top.height()),
                   Qt::AlignRight | Qt::AlignVCenter, index.data(CharAreaRole).toString());
    painter->setFont(serifFont(16));
    drawShadowText(*painter, QRect(rightLeft, bottom.top(), text.right() - rightLeft, bottom.height()),
                   Qt::AlignRight | Qt::AlignVCenter, formatPlayTime(index.data(CharPlayTimeRole).toUInt()));

    for (const int y : {top.bottom() - 1, bottom.bottom() - 1}) {
        drawGoldRule(*painter, text.left() - 4, text.left() + leftWidth, y, 210);
        drawGoldRule(*painter, rightLeft, text.right() + 4, y, 170);
    }
    painter->restore();
}

LoadGameFrame::LoadGameFrame(QWidget* content, QWidget* parent): QWidget(parent), m_content(content) {
    setAttribute(Qt::WA_TranslucentBackground, true);
    m_icon = QPixmap(":/ds3_images/menu_status");
    m_content->setParent(this);
    setMinimumWidth(300);
}

QSize LoadGameFrame::sizeHint() const {
    return {350, 400};
}

QRect LoadGameFrame::wellRect() const {
    return rect().adjusted(STONE_BORDER, TITLE_HEIGHT + STONE_BORDER, -STONE_BORDER, -STONE_BORDER);
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

    // Title with the menu icon, like "Characters" above the slots.
    const int iconSize = 26;
    if (!m_icon.isNull()) {
        painter.drawPixmap(QRect(4, (TITLE_HEIGHT - iconSize) / 2, iconSize, iconSize), m_icon);
    }
    painter.setFont(serifFont(18));
    drawShadowText(painter, QRect(iconSize + 11, 0, width() - iconSize - 13, TITLE_HEIGHT),
                   Qt::AlignLeft | Qt::AlignVCenter, "Characters");

    // Weathered stone frame.
    const QRect outer = rect().adjusted(0, TITLE_HEIGHT, 0, 0);
    QLinearGradient stone(outer.topLeft(), outer.bottomRight());
    stone.setColorAt(0.0, QColor(96, 86, 80));
    stone.setColorAt(0.5, QColor(70, 62, 58));
    stone.setColorAt(1.0, QColor(84, 72, 64));
    painter.setPen(Qt::NoPen);
    painter.setBrush(stone);
    painter.drawRect(outer);
    painter.drawTiledPixmap(outer, grainTexture());
    painter.drawTiledPixmap(outer, grainTexture(), QPoint(37, 59));
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(QColor(20, 16, 14), 1));
    painter.drawRect(outer.adjusted(0, 0, -1, -1));
    painter.setPen(QPen(QColor(170, 160, 150, 110), 1, Qt::DotLine));
    painter.drawRect(outer.adjusted(3, 3, -4, -4));

    // Dark teal well the slots sit in.
    const QRect well = wellRect();
    QLinearGradient teal(well.topLeft(), well.bottomLeft());
    teal.setColorAt(0.0, QColor(22, 40, 40));
    teal.setColorAt(0.6, QColor(36, 58, 58));
    teal.setColorAt(1.0, QColor(46, 70, 70));
    painter.setPen(Qt::NoPen);
    painter.setBrush(teal);
    painter.drawRect(well);
    painter.drawTiledPixmap(well, grainTexture());
    // Inner shadow along the top and left edges.
    QLinearGradient shadow(well.topLeft(), well.topLeft() + QPoint(0, 18));
    shadow.setColorAt(0.0, QColor(0, 0, 0, 150));
    shadow.setColorAt(1.0, QColor(0, 0, 0, 0));
    painter.setBrush(shadow);
    painter.drawRect(QRect(well.topLeft(), QSize(well.width(), 18)));
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(QColor(10, 14, 14), 2));
    painter.drawRect(well.adjusted(0, 0, -1, -1));
}
}
