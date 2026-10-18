#include "CharacterList.h"

#include <QAbstractItemView>
#include <QPainter>
#include <QPaintEvent>

#include "Painting.h"

namespace fssm::ui::ds2 {
namespace {
    // ui_data_list.png is 910x640; borders kept unscaled by the nine-slice.
    // Frame and title keep one size (the look at the default window size).
    constexpr double FIXED_SCALE = 0.45;
    constexpr int FRAME_LEFT = 50;
    constexpr int FRAME_TOP = 96;
    constexpr int FRAME_RIGHT = 50;
    constexpr int FRAME_BOTTOM = 76;
    constexpr int ROW_HEIGHT = 54;

    QString formatPlayTime(uint32_t seconds) {
        return QString("%1:%2:%3").arg(seconds / 3600)
                                  .arg((seconds / 60) % 60, 2, 10, QChar('0'))
                                  .arg(seconds % 60, 2, 10, QChar('0'));
    }
}

CharacterDelegate::CharacterDelegate(QObject* parent): QStyledItemDelegate(parent) {
    m_selectedBand = QPixmap(":/ds2_images/ui_row_selected");
}

QSize CharacterDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex&) const {
    return {option.rect.width(), ROW_HEIGHT};
}

void CharacterDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    painter->save();
    painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
    // Rows can be laid out wider than the visible list; keep the text inside the viewport.
    QRect r = option.rect;
    if (const auto* view = qobject_cast<const QAbstractItemView*>(option.widget)) {
        r.setWidth(std::min(r.width(), view->viewport()->width() - r.left()));
    }
    const QVariant name = index.data(CharNameRole);
    const bool selected = option.state & QStyle::State_Selected;
    const bool hovered = option.state & QStyle::State_MouseOver;
    if (name.isValid() && (selected || hovered) && !m_selectedBand.isNull()) {
        painter->setOpacity(selected ? 1.0 : 0.35);
        painter->drawPixmap(r.adjusted(0, 1, 0, -1), m_selectedBand);
        painter->setOpacity(1.0);
    }
    painter->setPen(QPen(QColor(110, 98, 74, 160), 1));
    painter->drawLine(r.bottomLeft() + QPoint(4, 0), r.bottomRight() - QPoint(4, 0));

    const QRect text = r.adjusted(10, 4, -10, -4);
    if (!name.isValid()) {
        // Error message from the model.
        painter->setFont(serifFont(13));
        drawOutlinedText(*painter, text, Qt::AlignLeft | Qt::AlignVCenter,
                         index.data(Qt::DisplayRole).toString(), DIM_TEXT_COLOR);
        painter->restore();
        return;
    }
    const QRect top(text.left(), text.top(), text.width(), text.height() / 2);
    const QRect bottom(text.left(), top.bottom() + 1, text.width(), text.height() - top.height());
    const int nameWidth = text.width() * 55 / 100;

    painter->setFont(serifFont(17));
    drawOutlinedText(*painter, QRect(top.left(), top.top(), nameWidth, top.height()),
                     Qt::AlignLeft | Qt::AlignVCenter, name.toString(), TEXT_COLOR);
    painter->setFont(serifFont(14));
    drawOutlinedText(*painter, QRect(top.left() + nameWidth, top.top(), top.width() - nameWidth, top.height()),
                     Qt::AlignRight | Qt::AlignVCenter, index.data(CharAreaRole).toString(), TEXT_COLOR);
    drawOutlinedText(*painter, bottom, Qt::AlignLeft | Qt::AlignVCenter,
                     QString("Level  %1").arg(index.data(CharLevelRole).toUInt()), TEXT_COLOR);
    drawOutlinedText(*painter, bottom, Qt::AlignRight | Qt::AlignVCenter,
                     formatPlayTime(index.data(CharPlayTimeRole).toUInt()), TEXT_COLOR);
    painter->restore();
}

DataListFrame::DataListFrame(QWidget* content, QWidget* parent): QWidget(parent), m_content(content) {
    setAttribute(Qt::WA_TranslucentBackground, true);
    m_window = QPixmap(":/ds2_images/ui_data_list");
    m_content->setParent(this);
    setMinimumWidth(280);
}

double DataListFrame::scale() const {
    return FIXED_SCALE;
}

void DataListFrame::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    const double s = scale();
    m_content->setGeometry(rect().adjusted(int(FRAME_LEFT * s) + 2, int(FRAME_TOP * s) + 2,
                                           -(int(FRAME_RIGHT * s) + 2), -(int(FRAME_BOTTOM * s) + 2)));
}

void DataListFrame::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    painter.setClipRect(event->rect());
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    const double s = scale();
    if (!m_window.isNull()) {
        drawNineSlice(painter, m_window, rect(), FRAME_LEFT, FRAME_TOP, FRAME_RIGHT, FRAME_BOTTOM, s);
    }
    // Title in the blue bar, next to the window's own icon.
    const QRect title(int(118 * s), int(18 * s), width() - int(160 * s), int(36 * s));
    painter.setFont(serifFont(std::max(17, int(32 * s))));
    drawOutlinedText(painter, title, Qt::AlignLeft | Qt::AlignVCenter, "Data List", TEXT_COLOR);
}
}
