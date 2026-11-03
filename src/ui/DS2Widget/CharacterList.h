#pragma once

#include <QPixmap>
#include <QStyledItemDelegate>
#include <QWidget>

namespace fssm::ui::ds2 {
const int CharIdRole = Qt::UserRole + 1;
const int CharNameRole = Qt::UserRole + 2;
const int CharLevelRole = Qt::UserRole + 3;
const int CharAreaRole = Qt::UserRole + 4;
const int CharPlayTimeRole = Qt::UserRole + 5;

// Character row drawn like the game's "Data List" (character select) screen:
// name and level on the left, area and play time on the right.
class CharacterDelegate: public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit CharacterDelegate(QObject* parent);
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
private:
    QPixmap m_selectedBand;
};

// The game's "Data List" window drawn around `content`.
class DataListFrame: public QWidget {
    Q_OBJECT
public:
    explicit DataListFrame(QWidget* content, QWidget* parent);
protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
private:
    double scale() const;
    QPixmap m_window;
    QWidget* m_content;
};
}
