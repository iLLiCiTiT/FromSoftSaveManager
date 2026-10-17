#pragma once

#include <QStyledItemDelegate>

#include "Painting.h"

namespace fssm::ui::sekiro {
const int CharIdRole = Qt::UserRole + 1;
const int CharAreaRole = Qt::UserRole + 2;      // null for empty slots
const int CharPlayTimeRole = Qt::UserRole + 3;
const int CharLastSavedRole = Qt::UserRole + 4; // unix seconds
const int CharVitalityRole = Qt::UserRole + 5;
const int CharAttackPowerRole = Qt::UserRole + 6;

// Character row drawn like a profile on the game's "Load Game" screen: an ink
// stroke with the save time and area on top, play time and stats below.
class CharacterDelegate: public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit CharacterDelegate(QObject* parent);
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
private:
    mutable PixmapCache m_cache;
};
}
