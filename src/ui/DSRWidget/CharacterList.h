#pragma once

#include <QPixmap>
#include <QStyledItemDelegate>
#include <QWidget>

namespace fssm::ui::dsr {
const int CharIdRole = Qt::UserRole + 1;
const int CharNameRole = Qt::UserRole + 2;
const int CharLevelRole = Qt::UserRole + 3;
const int CharPlayTimeRole = Qt::UserRole + 4;
const int CharAreaRole = Qt::UserRole + 5;

// Character row drawn like a slot on the game's "Data List" screen: a carved gold frame
// around a leather plate, name over the level on the left, area over the play time on the right.
class CharacterDelegate: public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit CharacterDelegate(QObject* parent);
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
};

// The game's menu heading ("Characters") above `content`, which sits in a framed panel.
class DataListFrame: public QWidget {
    Q_OBJECT
public:
    explicit DataListFrame(QWidget* content, QWidget* parent);
    QSize sizeHint() const override;
protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
private:
    QRect panelRect() const;
    QWidget* m_content;
    QPixmap m_icon;
};
}
