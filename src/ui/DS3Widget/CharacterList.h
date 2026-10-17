#pragma once

#include <QStyledItemDelegate>
#include <QWidget>

namespace fssm::ui::ds3 {
const int CharIdRole = Qt::UserRole + 1;
const int CharNameRole = Qt::UserRole + 2;
const int CharLevelRole = Qt::UserRole + 3;
const int CharAreaRole = Qt::UserRole + 4;
const int CharPlayTimeRole = Qt::UserRole + 5;

// Character row drawn like a save slot on the game's "Load Game" screen:
// a slate plate with name and level on the left, area and play time on the right.
class CharacterDelegate: public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit CharacterDelegate(QObject* parent);
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
};

// The "Load Game" window (title, stone frame, dark teal well) drawn around `content`.
class LoadGameFrame: public QWidget {
    Q_OBJECT
public:
    explicit LoadGameFrame(QWidget* content, QWidget* parent);
    QSize sizeHint() const override;
protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
private:
    QRect wellRect() const;
    QWidget* m_content;
    QPixmap m_icon;
};
}
