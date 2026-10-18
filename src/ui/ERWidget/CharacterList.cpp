#include "CharacterList.h"

#include <QAbstractItemView>
#include <QPaintEvent>
#include <QPainter>
#include <cmath>

namespace fssm::ui::er {
namespace {
    constexpr int ROW_HEIGHT = 58;
    constexpr int TITLE_HEIGHT = 30;
    constexpr int PANEL_PADDING = 8;

    const QColor TEXT_COLOR(228, 224, 210);
    const QColor DIM_TEXT_COLOR(150, 144, 128);
    // Colors sampled from the game's Load Game screen.
    const QColor SLOT_COLOR(13, 12, 10);
    const QColor SLOT_SELECTED_COLOR(31, 30, 25);
    const QColor BORDER_COLOR(95, 91, 82);
    const QColor BORDER_SELECTED_COLOR(128, 124, 111);
    const QColor RULE_COLOR(53, 51, 43);
    const QColor PANEL_COLOR(32, 31, 25, 210);

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
        painter.setPen(QColor(0, 0, 0, 200));
        painter.drawText(rect.translated(1, 1), flags, elided);
        painter.setPen(color);
        painter.drawText(rect, flags, elided);
        painter.restore();
    }

    // Thin grey rule under a label, fading out towards its far end.
    void drawRule(QPainter& painter, int x1, int x2, int y, bool fadeRight) {
        if (x2 <= x1) return;
        QLinearGradient gradient(x1, 0, x2, 0);
        QColor clear = RULE_COLOR;
        clear.setAlpha(0);
        gradient.setColorAt(0.0, fadeRight ? RULE_COLOR : clear);
        gradient.setColorAt(fadeRight ? 0.6 : 0.4, RULE_COLOR);
        gradient.setColorAt(1.0, fadeRight ? clear : RULE_COLOR);
        painter.save();
        painter.setPen(QPen(QBrush(gradient), 1));
        painter.drawLine(x1, y, x2, y);
        painter.restore();
    }

    QString formatPlayTime(uint32_t seconds) {
        return QString("%1:%2:%3").arg(seconds / 3600)
                                  .arg((seconds / 60) % 60, 2, 10, QChar('0'))
                                  .arg(seconds % 60, 2, 10, QChar('0'));
    }

    // Profile slot: near-black fill with a 1px grey frame and a faint line inside it;
    // the selected one is lighter with a brighter frame.
    void drawSlot(QPainter& painter, const QRect& r, bool selected, bool hovered) {
        painter.save();
        painter.setPen(Qt::NoPen);
        QLinearGradient fill(r.topLeft(), r.bottomLeft());
        const QColor base = selected ? SLOT_SELECTED_COLOR : SLOT_COLOR;
        fill.setColorAt(0.0, base.lighter(115));
        fill.setColorAt(1.0, base);
        painter.setBrush(fill);
        painter.drawRect(r);
        if (hovered && !selected) {
            painter.setBrush(QColor(141, 137, 123, 30));
            painter.drawRect(r);
        }
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(selected ? BORDER_SELECTED_COLOR : BORDER_COLOR, 1));
        painter.drawRect(r.adjusted(0, 0, -1, -1));
        QColor inner = selected ? BORDER_SELECTED_COLOR : BORDER_COLOR;
        inner.setAlpha(selected ? 110 : 60);
        painter.setPen(QPen(inner, 1));
        painter.drawRect(r.adjusted(2, 2, -3, -3));
        painter.restore();
    }
}

CharacterDelegate::CharacterDelegate(QObject* parent): QStyledItemDelegate(parent) {}

QSize CharacterDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex&) const {
    return {option.rect.width(), ROW_HEIGHT};
}

void CharacterDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, false);
    // Rows can be laid out wider than the visible list; keep them inside the viewport.
    QRect r = option.rect;
    if (const auto* view = qobject_cast<const QAbstractItemView*>(option.widget)) {
        r.setWidth(std::min(r.width(), view->viewport()->width() - r.left()));
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
    const QRect slot = r.adjusted(0, 3, 0, -3);
    drawSlot(*painter, slot, selected, hovered);

    // Two lines, each over a grey rule: name and area, then level and play time.
    const QRect text = slot.adjusted(12, 4, -12, -5);
    const int splitX = text.left() + text.width() * 45 / 100;
    const int lineHeight = text.height() / 2;
    const QRect top(text.left(), text.top(), text.width(), lineHeight);
    const QRect bottom(text.left(), text.top() + lineHeight, text.width(), text.height() - lineHeight);

    painter->setFont(serifFont(14));
    drawText(*painter, QRect(top.left(), top.top(), splitX - top.left() - 6, top.height()),
             Qt::AlignLeft | Qt::AlignVCenter, name.toString());
    painter->setFont(serifFont(13));
    drawText(*painter, QRect(splitX, top.top(), top.right() - splitX, top.height()),
             Qt::AlignRight | Qt::AlignVCenter, index.data(CharAreaRole).toString());
    drawText(*painter, QRect(bottom.left(), bottom.top(), splitX - bottom.left() - 6, bottom.height()),
             Qt::AlignLeft | Qt::AlignVCenter, QString("Level %1").arg(index.data(CharLevelRole).toUInt()),
             DIM_TEXT_COLOR);
    drawText(*painter, QRect(splitX, bottom.top(), bottom.right() - splitX, bottom.height()),
             Qt::AlignRight | Qt::AlignVCenter, formatPlayTime(index.data(CharPlayTimeRole).toUInt()),
             DIM_TEXT_COLOR);

    for (const QRect& line : {top, bottom}) {
        drawRule(*painter, line.left(), splitX - 6, line.bottom() - 1, true);
        drawRule(*painter, splitX, line.right() + 2, line.bottom() - 1, false);
    }
    painter->restore();
}

LoadGameFrame::LoadGameFrame(QWidget* content, QWidget* parent): QWidget(parent), m_content(content) {
    setAttribute(Qt::WA_TranslucentBackground, true);
    m_content->setParent(this);
    setMinimumWidth(250);
}

QSize LoadGameFrame::sizeHint() const {
    return {290, 400};
}

QRect LoadGameFrame::panelRect() const {
    return rect().adjusted(0, TITLE_HEIGHT, 0, 0);
}

void LoadGameFrame::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    m_content->setGeometry(panelRect().adjusted(PANEL_PADDING, PANEL_PADDING - 3, -PANEL_PADDING, -PANEL_PADDING + 3));
}

void LoadGameFrame::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    painter.setClipRect(event->rect());

    // Title above the profiles, where the game has "LOAD GAME".
    painter.setFont(serifFont(17));
    drawText(painter, QRect(2, 0, width() - 4, TITLE_HEIGHT), Qt::AlignLeft | Qt::AlignVCenter, "Characters");
    const QRect titleLine(0, TITLE_HEIGHT - 5, width(), 1);
    QLinearGradient line(titleLine.topLeft(), titleLine.topRight());
    line.setColorAt(0.0, QColor(RULE_COLOR.red(), RULE_COLOR.green(), RULE_COLOR.blue(), 220));
    line.setColorAt(1.0, QColor(RULE_COLOR.red(), RULE_COLOR.green(), RULE_COLOR.blue(), 0));
    painter.fillRect(titleLine, line);

    // Dim panel the slots sit on; its edges fade into the background. Stacked
    // translucent rects, each one inset further, build up the full color inside.
    constexpr int FEATHER = 8;
    const QRect panel = panelRect();
    QColor layer = PANEL_COLOR;
    layer.setAlphaF(1.0 - std::pow(1.0 - PANEL_COLOR.alphaF(), 1.0 / FEATHER));
    painter.setPen(Qt::NoPen);
    painter.setBrush(layer);
    for (int i = 0; i < FEATHER; ++i) {
        painter.drawRect(panel.adjusted(i, i, -i, -i));
    }
}
}
