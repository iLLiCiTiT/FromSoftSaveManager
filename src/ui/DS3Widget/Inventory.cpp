#include "Inventory.h"

#include <QApplication>
#include <QPainter>
#include <QListView>
#include <QScrollBar>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>
#include <QStyledItemDelegate>
#include <QVariantAnimation>

#include "../Utils.h"
#include "../../parse/Parse.h"

namespace fssm::ui::ds3 {
InventoryModel::InventoryModel(QObject* parent): QStandardItemModel(parent) {}


void InventoryModel::setCharacter(const fssm::parse::ds3::DS3CharacterInfo* charInfo) {
    QStandardItem* rootItem = invisibleRootItem();
    rootItem->removeRows(0, rootItem->rowCount());
    if (charInfo == nullptr) return;

    // std::unordered_map<uint32_t, QStandardItem*> itemsById;
    // QList<QStandardItem*> newItems;
    // for (auto invItem: charInfo->inventoryItems) {
    //     QStandardItem* item = createModelItem(invItem);
    //     if (item == nullptr) continue;
    //
    //     item->setData(invItem.amount, ITEM_AMOUNT_ROLE);
    //     // Show consumables, materials and ammunition items from both inventory and bottomless box in one item
    //     // TODO use enum for category
    //     if (invItem.baseItem.category == "consumables" || invItem.baseItem.category == "materials" || invItem.baseItem.category == "ammunition") {
    //         itemsById[invItem.itemId] = item;
    //     }
    //     newItems.append(item);
    // }
    // for (auto blbItem: charInfo->botomlessBoxItems) {
    //     QStandardItem* item = nullptr;
    //     if (blbItem.baseItem.category == "consumables" || blbItem.baseItem.category == "materials" || blbItem.baseItem.category == "ammunition") {
    //         auto iterItem = itemsById.find(blbItem.itemId);
    //         if (iterItem != itemsById.end()) item = iterItem->second;
    //     };
    //     if (item == nullptr) {
    //         item = createModelItem(blbItem);
    //         if (item == nullptr) continue;
    //         itemsById[blbItem.itemId] = item;
    //         newItems.append(item);
    //     }
    //     item->setData(blbItem.amount, ITEM_BOTOMLESS_BOX_AMOUNT_ROLE);
    // }
    //
    // if (!newItems.isEmpty())
    //     rootItem->appendRows(newItems);
}

static QPixmap getInfusionIcon(const uint16_t& infusion, const uint8_t& upgradeLevel) {
    if (!hasDS3InventoryResources()) return QPixmap{};
    switch (infusion) {
        case 100:
            return QPixmap(":/ds3_inv_images/crystal.png");
        case 200:
            return QPixmap(":/ds3_inv_images/lightning.png");
        case 300:
            return QPixmap(":/ds3_inv_images/raw.png");
        case 400:
            if (upgradeLevel >= 5)
                return QPixmap(":/ds3_inv_images/magic_2.png");
            return QPixmap(":/ds3_inv_images/magic.png");
        case 500:
            return QPixmap(":/ds3_inv_images/enchanted.png");
        case 600:
            if (upgradeLevel >= 5)
                return QPixmap(":/ds3_inv_images/divine_2.png");
            return QPixmap(":/ds3_inv_images/divine.png");
        case 700:
            return QPixmap(":/ds3_inv_images/occult.png");
        case 800:
            if (upgradeLevel >= 5)
                return QPixmap(":/ds3_inv_images/fire_2.png");
            return QPixmap(":/ds3_inv_images/fire.png");

        case 900:
            return QPixmap(":/ds3_inv_images/chaos.png");
        default:
            return QPixmap{};
    }
}

static QPixmap getItemImage(const std::string_view& image) {
    if (!hasDS3InventoryResources()) return QPixmap{};

    QString imagePath = QString::fromStdString(":/ds3_inv_images/");
    imagePath.append(QString::fromStdString(image.data()));
    return QPixmap(imagePath);
}

QStandardItem* InventoryModel::createModelItem(fssm::parse::ds3::InventoryItem& inventoryItem) {
    // if (!inventoryItem.knownItem) return createUnknownItem(inventoryItem);
    // // Skip fist
    // if (inventoryItem.baseItem.type == 0 && inventoryItem.baseItem.id == 900000) return nullptr;
    // // Skip no armor
    // if (inventoryItem.baseItem.type == 268435456) {
    //     switch (inventoryItem.baseItem.id) {
    //         case 900000:
    //         case 901000:
    //         case 902000:
    //         case 903000:
    //             return nullptr;
    //         default:
    //             break;
    //     }
    // }
    //
    // QPixmap infusionPix = getInfusionIcon(inventoryItem.infusion, inventoryItem.upgradeLevel);
    // QPixmap itemImage = getItemImage(inventoryItem.baseItem.image);

    QStandardItem* item = new QStandardItem(QString::fromStdString(inventoryItem.baseItem.label.data()));
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    // item->setData(inventoryItem.upgradeLevel, ITEM_LEVEL_ROLE);
    // item->setData(infusionPix, ITEM_INFUSION_ICON_ROLE);
    // item->setData(inventoryItem.order, ITEM_ORDER_ROLE);
    // item->setData(inventoryItem.durability, ITEM_DURABILITY_ROLE);
    // item->setData(0, ITEM_AMOUNT_ROLE);
    // item->setData(0, ITEM_BOTOMLESS_BOX_AMOUNT_ROLE);
    // item->setData(itemImage, ITEM_IMAGE_ROLE);
    // item->setData(QString::fromStdString(inventoryItem.baseItem.category.data()), ITEM_CATEGORY_ROLE);
    return item;
}

QStandardItem* InventoryModel::createUnknownItem(fssm::parse::ds3::InventoryItem& inventoryItem) {
    QStandardItem* item = new QStandardItem();
    QString label;
    label.push_back("NA ");
    label.push_back(QString::fromStdString(std::to_string(inventoryItem.itemId)));
    item->setText(label);

    item->setData(inventoryItem.upgradeLevel, ITEM_LEVEL_ROLE);
    // item->setData(inventoryItem.order, ITEM_ORDER_ROLE);
    // item->setData(inventoryItem.durability, ITEM_DURABILITY_ROLE);
    // item->setData(QVariant(QString::fromStdString(inventoryItem.baseItem.category.data())), ITEM_CATEGORY_ROLE);
    item->setData(QPixmap(":/ds3_images/unknown.png"), ITEM_IMAGE_ROLE);
    return item;
}

InventoryProxyModel::InventoryProxyModel(QObject *parent): QSortFilterProxyModel(parent) {
    setSortCaseSensitivity(Qt::CaseInsensitive);
    setFilterCaseSensitivity(Qt::CaseInsensitive);
}

void InventoryProxyModel::setCategory(parse::ds3::ItemCategory category) {
    if (category == m_category) return;
    beginFilterChange();
    m_category = category;
    endFilterChange(Direction::Rows);
}

bool InventoryProxyModel::lessThan(const QModelIndex &sourceLeft, const QModelIndex &sourceRight) const {
    int left = sourceLeft.data(ITEM_ORDER_ROLE).toInt();
    int right = sourceRight.data(ITEM_ORDER_ROLE).toInt();
    if (left != right) return left < right;
    return QSortFilterProxyModel::lessThan(sourceLeft, sourceRight);
}

bool InventoryProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const {
    QModelIndex index = sourceModel()->index(source_row, 0, source_parent);
    parse::ds3::ItemCategory category = index.data(ITEM_CATEGORY_ROLE).value<parse::ds3::ItemCategory>();
    return category == m_category;
}

InventoryDelegate::InventoryDelegate(QObject* parent): QStyledItemDelegate(parent) {
    m_standPix = QPixmap(":/ds3_images/inventory_stand.png");
    m_inventoryBagPix = QPixmap(":/ds3_images/inventory_bag.png");
    m_bottomlessBoxPix = QPixmap(":/ds3_images/bottomless_box.png");
}

int InventoryDelegate::paintIcon(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    int textOffset = 20;
    if (!hasDS3InventoryResources()) return textOffset;

    QVariant pixmapValue = index.data(ITEM_IMAGE_ROLE);
    QPixmap pixmap;
    if (pixmapValue.isValid() && !pixmapValue.isNull()) {
        switch (pixmapValue.userType()) {
            case QMetaType::QPixmap:
                pixmap = qvariant_cast<QPixmap>(pixmapValue);
                break;
            default:
                break;
        }
    }
    if (pixmap.isNull())
        pixmap = QPixmap(":/ds3_images/unknown.png");
    int imgSize = option.rect.height() - 20;
    pixmap = pixmap.scaled(
        imgSize, imgSize,
        Qt::KeepAspectRatio, Qt::SmoothTransformation
    );
    QPixmap standPix = m_standPix.scaled(
        pixmap.width(), pixmap.width(),
        Qt::KeepAspectRatio, Qt::SmoothTransformation
    );

    QRect iconRect = option.rect;
    iconRect.setWidth(pixmap.width());
    textOffset += iconRect.right();
    iconRect.adjust(10, 10, 10, -10);

    QRect standRect = QRect(
        iconRect.left(),
        (iconRect.bottom() - standPix.height()) + 10,
        standPix.width(),
        standPix.height()
    );

    painter->drawPixmap(standRect, standPix);
    painter->drawPixmap(iconRect, pixmap);

    QVariant infusionIconValue = index.data(ITEM_INFUSION_ICON_ROLE);
    QPixmap infusionIcon;
    if (infusionIconValue.isValid()) {
        infusionIcon = qvariant_cast<QPixmap>(infusionIconValue);
    }
    if (!infusionIcon.isNull()) {
        int infusionSize = int(option.rect.height() * 0.3);
        infusionIcon = infusionIcon.scaled(
            infusionSize,
            infusionSize,
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation
        );
        QRect infusionRect = QRect(
            (iconRect.right() - infusionIcon.width()) + 4,
            (iconRect.bottom() - infusionIcon.height()) + 4,
            infusionIcon.width(),
            infusionIcon.height()
        );
        painter->drawPixmap(infusionRect, infusionIcon);
    }
    return textOffset;
}

void InventoryDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);
    const QStyle* style = (option.widget ? option.widget->style() : QApplication::style())->proxy();

    style->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, painter, option.widget);

    int textOffset = paintIcon(painter, option, index);

    QString label = index.data(Qt::DisplayRole).toString();
    QVariant levelValue = index.data(ITEM_LEVEL_ROLE);
    if (levelValue.isValid() && !levelValue.isNull()) {
        int level = levelValue.toInt();
        if (level > 0)
            label.append("+" + QString::number(level));
    }

    QFont font = painter->font();
    font.setPointSize(12);
    painter->setFont(font);

    QFontMetrics fm(font);
    int textHeight = fm.height();
    int halfHeight = option.rect.height() / 2;
    

    QRect textRect = option.rect.adjusted(
        textOffset, halfHeight - textHeight, 0, 0
    );
    textRect.setHeight(textHeight);

    painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, label);

    QPixmap invBagPix = m_inventoryBagPix.scaled(
        textHeight, textHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation
    );
    QPixmap btbPix = m_bottomlessBoxPix.scaled(
        textHeight, textHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation
    );

    QString amountText = QString::number(index.data(ITEM_AMOUNT_ROLE).toInt());
    QString bottomlessBoxText = QString::number(index.data(ITEM_BOTOMLESS_BOX_AMOUNT_ROLE).toInt());

    QPoint pos = QPoint(textRect.x(), option.rect.top() + halfHeight + 10);
    painter->drawPixmap(pos, invBagPix);
    pos.setX(pos.x() + invBagPix.width() + 2);

    QRect amountRect = fm.boundingRect("9999");
    amountRect.moveTopLeft(pos);
    painter->drawText(amountRect, Qt::AlignLeft | Qt::AlignVCenter, amountText);
    pos.setX(pos.x() + amountRect.width() + 5);

    painter->drawPixmap(pos, btbPix);
    pos.setX(pos.x() + btbPix.width() + 2);

    amountRect.moveLeft(pos.x());
    painter->drawText(amountRect, Qt::AlignLeft | Qt::AlignVCenter, bottomlessBoxText);
}

QSize InventoryDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const {
    return QSize(260, 80);
}

DS3InventoryCategoryButton::DS3InventoryCategoryButton(const fssm::parse::ds3::ItemCategory& category, QWidget* parent): BaseClickableFrame(parent), m_category(category) {
    setAttribute(Qt::WA_TranslucentBackground, true);
    m_pix = QPixmap(":/ds3_images/" + getCategoryIcon(category));
    m_bg1 = QPixmap(":/ds3_images/inv_bg_1");
    m_bg2 = QPixmap(":/ds3_images/inv_bg_2");
    m_imageLabel = new PixmapLabel(m_pix, this);
    m_imageLabel->setObjectName("ds3_category_icon");
    m_imageLabel->setAttribute(Qt::WA_TranslucentBackground, true);

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(5, 5, 5, 5);
    layout->addWidget(m_imageLabel, 1);
};

void DS3InventoryCategoryButton::setSelected(bool selected) {
    if (selected == m_isSelected) return;
    m_isSelected = selected;
    updateGeometry();
    repaint();
}

void DS3InventoryCategoryButton::onMouseRelease() {
    emit clicked(m_category);
}

QSize DS3InventoryCategoryButton::sizeHint() const {
    return minimumSizeHint();
};

QSize DS3InventoryCategoryButton::minimumSizeHint() const {
    if (m_isSelected) return QSize(60, 60);
    return QSize(30, 60);
};

void DS3InventoryCategoryButton::enterEvent(QEnterEvent *event) {
    m_isHovered = true;
    repaint();
}

void DS3InventoryCategoryButton::leaveEvent(QEvent *event) {
    m_isHovered = false;
    repaint();
}

void DS3InventoryCategoryButton::paintEvent(QPaintEvent *)  {
    QPainter painter = QPainter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    painter.setBrush(Qt::NoBrush);
    painter.setClipRect(rect());
    if (m_isSelected) {
        painter.drawPixmap(0, 0, m_bg1.scaled(width(), height(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
    } else if (m_isHovered) {
        painter.drawPixmap(0, 0, m_bg2.scaled(width(), height(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
    }
    QPixmap scaled = m_pix.scaled(size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    // center vertically
    const int y = (height() - scaled.height()) / 2;
    painter.drawPixmap(0, y, scaled);
}

CategoryButtons::CategoryButtons(QWidget* parent): QWidget(parent) {
    setAttribute(Qt::WA_TranslucentBackground, true);

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    for (auto category : std::initializer_list {
        parse::ds3::ItemCategory::Tools,
        parse::ds3::ItemCategory::Materials,
        parse::ds3::ItemCategory::KeyItems,
        parse::ds3::ItemCategory::Spells,
        parse::ds3::ItemCategory::MeleeWeapons,
        parse::ds3::ItemCategory::RangedWeapons,
        parse::ds3::ItemCategory::Catalysts,
        parse::ds3::ItemCategory::Shields,
        parse::ds3::ItemCategory::HeadArmor,
        parse::ds3::ItemCategory::ChestArmor,
        parse::ds3::ItemCategory::HandsArmor,
        parse::ds3::ItemCategory::LegsArmor,
        parse::ds3::ItemCategory::ArrowsBolts,
        parse::ds3::ItemCategory::Rings,
        parse::ds3::ItemCategory::CovenantItem,
    }) {
        DS3InventoryCategoryButton* btn = new DS3InventoryCategoryButton(category, this);
        connect(btn, SIGNAL(clicked(parse::ds3::ItemCategory)), this, SLOT(setCategory(parse::ds3::ItemCategory)));
        if (m_category == category) btn->setSelected(true);
        m_categoryMapping[category] = btn;
        layout->addWidget(btn, 0);
    }
    layout->addStretch(1);
}

parse::ds3::ItemCategory CategoryButtons::getCategory() {
    return m_category;
}

void CategoryButtons::setCategory(parse::ds3::ItemCategory category) {
    if (m_category == category) return;
    m_categoryMapping[m_category]->setSelected(false);
    m_category = category;
    m_categoryMapping[m_category]->setSelected(true);
    emit categoryChanged(category);
}

InventoryWidget::InventoryWidget(QWidget* parent): QWidget(parent) {
    m_categoryBtns = new CategoryButtons(this);

    m_view = new QListView(this);
    m_view->setObjectName("ds_list_view");
    m_view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_view->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_view->verticalScrollBar()->setSingleStep(15);

    m_delegate = new InventoryDelegate(this);
    m_view->setItemDelegate(m_delegate);
    m_model = new InventoryModel(this);
    m_proxy = new InventoryProxyModel(this);
    m_proxy->setSourceModel(m_model);
    m_view->setModel(m_proxy);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_categoryBtns, 0);
    layout->addWidget(m_view, 1);

    connect(m_categoryBtns, SIGNAL(categoryChanged(parse::ds3::ItemCategory)), this, SLOT(onCategoryChange(parse::ds3::ItemCategory)));

    m_proxy->setCategory(m_categoryBtns->getCategory());
}

void InventoryWidget::setCharacter(const fssm::parse::ds3::DS3CharacterInfo* charInfo) {
    m_model->setCharacter(charInfo);
    m_proxy->sort(0, Qt::AscendingOrder);
}

void InventoryWidget::onCategoryChange(parse::ds3::ItemCategory category) {
    m_proxy->setCategory(category);
}
}
