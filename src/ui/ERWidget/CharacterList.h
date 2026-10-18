#pragma once

#include <QStyledItemDelegate>
#include <QWidget>

namespace fssm::ui::er {
const int CharIdRole = Qt::UserRole + 1;
const int CharNameRole = Qt::UserRole + 2;
const int CharLevelRole = Qt::UserRole + 3;
const int CharPlayTimeRole = Qt::UserRole + 4;

// Character row drawn like a profile on the game's "Load Game" screen: a near-black
// slot with a thin grey border holding name, level and play time on one line.
class CharacterDelegate: public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit CharacterDelegate(QObject* parent);
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
};

// Title and the dim panel the profiles sit on, drawn around `content`.
class LoadGameFrame: public QWidget {
    Q_OBJECT
public:
    explicit LoadGameFrame(QWidget* content, QWidget* parent);
    QSize sizeHint() const override;
protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
private:
    QRect panelRect() const;
    QWidget* m_content;
};
}
