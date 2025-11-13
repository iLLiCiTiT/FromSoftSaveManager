#pragma once
#include <QListView>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>
#include <QStyledItemDelegate>
#include <QVariantAnimation>

#include "../Utils.h"
#include "../../parse/Parse.h"

namespace fssm::ui::dsr {
const int ItemLevelRole = Qt::UserRole + 1;
const int ItemInfusionIconRole = Qt::UserRole + 2;
const int ItemOrderRole = Qt::UserRole + 3;
const int ItemAmountRole = Qt::UserRole + 4;
const int ItemDurabilityRole = Qt::UserRole + 5;
const int ItemBottomlessBoxAmountRole = Qt::UserRole + 6;
const int ItemImageRole = Qt::UserRole + 7;
const int ItemCategoryRole = Qt::UserRole + 8;


class InventoryModel: public QStandardItemModel {
    Q_OBJECT
public:
    explicit InventoryModel(QObject* parent = nullptr);
    void setCharacter(const fssm::parse::dsr::DSRCharacterInfo* charInfo);
private:
    QStandardItem* createModelItem(fssm::parse::dsr::InventoryItem& inventoryItem);
    QStandardItem* createUnknownItem(fssm::parse::dsr::InventoryItem& inventoryItem);
};

class InventoryProxyModel: public QSortFilterProxyModel {
    Q_OBJECT
public:
    explicit InventoryProxyModel(QObject *parent = nullptr);
    void setCategory(parse::dsr::ItemCategory category);
    bool lessThan(const QModelIndex &sourceLeft, const QModelIndex &sourceRight) const override;
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;
private:
    parse::dsr::ItemCategory m_category = parse::dsr::ItemCategory::Spells;
};

class InventoryDelegate: public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit InventoryDelegate(QObject* parent = nullptr);
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
private:
    QPixmap m_standPix;
    QPixmap m_inventoryBagPix;
    QPixmap m_bottomlessBoxPix;
    int paintIcon(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;
};

class DSRInventoryCategoryButton: public BaseClickableFrame {
    Q_OBJECT
signals:
    void clicked(parse::dsr::ItemCategory category);
public:
    explicit DSRInventoryCategoryButton(const parse::dsr::ItemCategory& category, QWidget* parent);
    void setSelected(bool selected);
protected:
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
private:
    void onMouseRelease() override;
    PixmapLabel* m_imageLabel = nullptr;
    parse::dsr::ItemCategory m_category;
    QPixmap m_pix;
    QPixmap m_hoverPix;
    bool m_isSelected = false;
    bool m_isHovered = false;
};

class CategoryButtonOverlay: public QWidget {
    Q_OBJECT
public:
    explicit CategoryButtonOverlay(QWidget* parent);
    void paintEvent(QPaintEvent* event) override;
private:
    QPixmap m_bgPix;
};

class CategoryButtons: public QWidget {
    Q_OBJECT
signals:
    void categoryChanged(parse::dsr::ItemCategory category);
public:
    explicit CategoryButtons(QWidget* parent);
    parse::dsr::ItemCategory getCategory() { return m_category; }
protected:
    void showEvent(QShowEvent* event) override;
    void resizeEvent(QResizeEvent *event) override;
public slots:
    void setCategory(parse::dsr::ItemCategory category);
private slots:
    void onAnimValueChange(QVariant posValue);
    void onAnimfinished();
private:
    CategoryButtonOverlay* m_overlayWidget = nullptr;
    QVariantAnimation* m_overlayAnim;
    parse::dsr::ItemCategory m_category;
    std::unordered_map<parse::dsr::ItemCategory, DSRInventoryCategoryButton*> m_categoryMapping;
};

class InventoryWidget: public QWidget {
    Q_OBJECT
public:
    explicit InventoryWidget(QWidget* parent);
    void setCharacter(const fssm::parse::dsr::DSRCharacterInfo* charInfo);
private slots:
    void onCategoryChange(parse::dsr::ItemCategory category);
private:
    CategoryButtons* m_categoryBtns = nullptr;
    QListView* m_view = nullptr;
    InventoryModel* m_model = nullptr;
    InventoryProxyModel* m_proxy = nullptr;
    InventoryDelegate* m_delegate = nullptr;
};
}