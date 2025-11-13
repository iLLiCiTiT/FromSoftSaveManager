#pragma once
#include <QListView>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>
#include <QStyledItemDelegate>
#include <QVariantAnimation>

#include "../Utils.h"
#include "../../parse/Parse.h"

namespace fssm::ui::ds3 {
const int ItemLevelRole = Qt::UserRole + 1;
const int ItemInfusionIconRole = Qt::UserRole + 2;
const int ItemOrderRole = Qt::UserRole + 3;
const int ItemAmountRole = Qt::UserRole + 4;
const int ItemDurabilityRole = Qt::UserRole + 5;
const int ItemStorageBoxAmountRole = Qt::UserRole + 6;
const int ItemImageRole = Qt::UserRole + 7;
const int ItemCategoryRole = Qt::UserRole + 8;

class InventoryModel: public QStandardItemModel {
    Q_OBJECT
public:
    explicit InventoryModel(QObject* parent = nullptr);
    void setCharacter(const parse::ds3::DS3CharacterInfo* charInfo);
private:
    QStandardItem* createModelItem(const parse::ds3::DS3CharacterInfo* charInfo, parse::ds3::InventoryItem& inventoryItem);
    QStandardItem* createUnknownItem(parse::ds3::InventoryItem& inventoryItem);
};

class InventoryProxyModel: public QSortFilterProxyModel {
    Q_OBJECT
public:
    explicit InventoryProxyModel(QObject *parent = nullptr);
    void setCategory(parse::ds3::ItemCategory category);
    bool lessThan(const QModelIndex &sourceLeft, const QModelIndex &sourceRight) const override;
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;
private:
    parse::ds3::ItemCategory m_category = parse::ds3::ItemCategory::Tools;
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

class DS3InventoryCategoryButton: public BaseClickableFrame {
    Q_OBJECT
signals:
    void clicked(parse::ds3::ItemCategory category);
public:
    explicit DS3InventoryCategoryButton(const parse::ds3::ItemCategory& category, QWidget* parent);
    void setSelected(bool selected);
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
protected:
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void paintEvent(QPaintEvent *) override;
private:
    void onMouseRelease() override;
    parse::ds3::ItemCategory m_category;
    QPixmap m_pix;
    QPixmap m_bg1;
    QPixmap m_bg2;
    bool m_isSelected = false;
    bool m_isHovered = false;
};

class CategoryButtons: public QWidget {
    Q_OBJECT
signals:
    void categoryChanged(parse::ds3::ItemCategory category);
public:
    explicit CategoryButtons(QWidget* parent);
    parse::ds3::ItemCategory getCategory();
public slots:
    void setCategory(parse::ds3::ItemCategory category);
private:
    parse::ds3::ItemCategory m_category = parse::ds3::ItemCategory::Tools;
    std::unordered_map<parse::ds3::ItemCategory, DS3InventoryCategoryButton*> m_categoryMapping;
};

class InventoryWidget: public QWidget {
    Q_OBJECT
public:
    explicit InventoryWidget(QWidget* parent);
    void setCharacter(const parse::ds3::DS3CharacterInfo* charInfo);
private slots:
    void onCategoryChange(parse::ds3::ItemCategory category);
private:
    CategoryButtons* m_categoryBtns = nullptr;
    QListView* m_view = nullptr;
    InventoryModel* m_model = nullptr;
    InventoryProxyModel* m_proxy = nullptr;
    InventoryDelegate* m_delegate = nullptr;
};
}
