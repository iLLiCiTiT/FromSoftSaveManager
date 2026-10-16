#pragma once

#include <QHash>
#include <QPixmap>
#include <QWidget>
#include <vector>

#include "../../parse/EldenRing/SaveFile.h"

namespace fssm::ui::er {
// Inventory screen drawn like the game's: category tabs, an item grid split
// into sort groups by separator lines (as many columns as fit, at least the
// game's five) and a strip under it with the selected item's details.
class InventoryWidget: public QWidget {
    Q_OBJECT
public:
    explicit InventoryWidget(QWidget* parent = nullptr);
    void setCharacter(const fssm::parse::er::ERCharacterInfo* charInfo);
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    struct Entry {
        fssm::parse::er::InventoryItem item;
        uint32_t stored = 0;        // same item in the storage chest (goods only)
        int upgradeLevel = 0;       // weapons
        QString title;
    };

    // A grid row; a new sort group always starts on a new row.
    struct Row {
        int first = 0;              // index into m_visible
        int count = 0;
        bool groupStart = false;    // separator line above (not on the first row)
        int top = 0;                // y offset inside the grid content
    };

    struct Layout {
        double scale = 1.0;
        QRect panel;
        QRect tabs;
        int tabWidth = 0;
        int visibleTabs = 5;
        QRect leftButton;
        QRect rightButton;
        QRect grid;
        int columns = 5;
        int cellWidth = 0;
        int rowHeight = 0;
        int separatorGap = 0;
        QRect detail;               // selected item, under the grid
    };

    Layout computeLayout() const;
    void rebuildVisibleItems();
    void layoutRows();
    void selectCategory(int category);
    void keepCategoryVisible();
    void selectItem(int index);
    int contentHeight() const;
    int maxScroll() const;
    int itemIndexAt(const QPoint& pos) const;
    int tabIndexAt(const QPoint& pos) const;
    QRect cellRect(int row, int column) const;
    const Entry* currentEntry() const;
    QPixmap scaled(const QString& resource, const QSize& size);
    QPixmap itemIcon(const Entry& entry, const QSize& size);
    void drawTabs(QPainter& painter);
    void drawGrid(QPainter& painter);
    void drawDetail(QPainter& painter);

    std::vector<Entry> m_entries;
    std::vector<const Entry*> m_visible;
    std::vector<Row> m_rows;
    bool m_hasCharacter = false;
    int m_category = 0;             // index into the tab list
    int m_tabOffset = 0;            // first visible tab
    int m_scroll = 0;               // pixels
    int m_hoverItem = -1;
    int m_hoverTab = -1;            // index into the tab list
    int m_selectedItem = 0;

    Layout m_layout;
    QHash<QString, QPixmap> m_scaledCache;
};
}
