#include "CharacterList.h"

#include <QAbstractItemView>
#include <QDateTime>
#include <QPainter>

namespace fssm::ui::sekiro {
namespace {
    constexpr int ROW_HEIGHT = 84;
}

CharacterDelegate::CharacterDelegate(QObject* parent): QStyledItemDelegate(parent) {}

QSize CharacterDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex&) const {
    return {option.rect.width(), ROW_HEIGHT};
}

void CharacterDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    painter->save();
    painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
    QRect r = option.rect;
    if (const auto* view = qobject_cast<const QAbstractItemView*>(option.widget)) {
        r.setWidth(std::min(r.width(), view->viewport()->width() - r.left()));
    }
    const QVariant area = index.data(CharAreaRole);
    if (!area.isValid()) {
        // Error message from the model.
        painter->setFont(serifFont(14));
        drawShadowText(*painter, r.adjusted(10, 0, -10, 0), Qt::AlignLeft | Qt::AlignVCenter,
                       index.data(Qt::DisplayRole).toString(), DIM_TEXT_COLOR);
        painter->restore();
        return;
    }

    const bool selected = option.state & QStyle::State_Selected;
    const bool hovered = option.state & QStyle::State_MouseOver;
    const QRect stroke = r.adjusted(0, 3, 0, -3);
    painter->drawPixmap(stroke.topLeft(), m_cache.get(":/sekiro_images/profile_row", stroke.size()));
    if (selected || hovered) {
        painter->setOpacity(selected ? 1.0 : 0.4);
        painter->drawPixmap(stroke.topLeft(), m_cache.get(":/sekiro_images/profile_row_selected", stroke.size()));
        painter->setOpacity(1.0);
    }

    // The game's profile row (save time and area over play time and stats) on
    // three lines, so it fits a narrow list: area and play time, save time, stats.
    const QRect text = stroke.adjusted(std::max(16, r.width() / 14), 6, -std::max(14, r.width() / 14), -6);
    const int lineH = text.height() / 3;
    const QRect line1(text.left(), text.top(), text.width(), lineH);
    const QRect line2(text.left(), line1.bottom() + 1, text.width(), lineH);
    const QRect line3(text.left(), line2.bottom() + 1, text.width(), text.height() - 2 * lineH);
    painter->setPen(QPen(QColor(240, 228, 200, 90), 1));
    painter->drawLine(QPoint(text.left(), line1.bottom() + 1), QPoint(text.right(), line1.bottom() + 1));

    painter->setFont(serifFont(15));
    drawShadowText(*painter, line1, Qt::AlignRight | Qt::AlignVCenter,
                   formatPlayTime(index.data(CharPlayTimeRole).toUInt()));
    drawShadowText(*painter, line1.adjusted(0, 0, -70, 0), Qt::AlignLeft | Qt::AlignVCenter, area.toString());

    painter->setFont(serifFont(13));
    const qint64 lastSaved = index.data(CharLastSavedRole).toLongLong();
    const QString saved = lastSaved > 0
        ? QDateTime::fromSecsSinceEpoch(lastSaved).toString("yyyy/MM/dd HH:mm:ss")
        : QString("-");
    drawShadowText(*painter, line2, Qt::AlignLeft | Qt::AlignVCenter, "Last Saved", DIM_TEXT_COLOR);
    drawShadowText(*painter, line2, Qt::AlignRight | Qt::AlignVCenter, saved);

    const int half = line3.width() * 42 / 100;
    const QRect vitality(line3.left(), line3.top(), half, line3.height());
    const QRect attack(line3.left() + half + 14, line3.top(), line3.width() - half - 14, line3.height());
    painter->setPen(QPen(QColor(240, 228, 200, 140), 1));
    painter->drawLine(QPoint(vitality.right() + 7, line3.top() + 3), QPoint(vitality.right() + 7, line3.bottom() - 2));
    drawShadowText(*painter, vitality, Qt::AlignLeft | Qt::AlignVCenter, "Vitality");
    drawShadowText(*painter, vitality, Qt::AlignRight | Qt::AlignVCenter,
                   QString::number(index.data(CharVitalityRole).toUInt()));
    drawShadowText(*painter, attack, Qt::AlignLeft | Qt::AlignVCenter, "Attack Power");
    drawShadowText(*painter, attack, Qt::AlignRight | Qt::AlignVCenter,
                   QString::number(index.data(CharAttackPowerRole).toUInt()));
    painter->restore();
}
}
