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

namespace fssm::ui::dsr {
InventoryModel::InventoryModel(QObject* parent): QStandardItemModel(parent) {}

void InventoryModel::setCharacter(const fssm::parse::dsr::DSRCharacterInfo* charInfo) {
    QStandardItem* rootItem = invisibleRootItem();
    rootItem->removeRows(0, rootItem->rowCount());
    if (charInfo == nullptr) return;

    std::unordered_map<uint32_t, QStandardItem*> itemsById;
    QList<QStandardItem*> newItems;
    for (auto invItem: charInfo->inventoryItems) {
        QStandardItem* item = createModelItem(invItem);
        if (item == nullptr) continue;

        item->setData(invItem.amount, ItemAmountRole);
        // Show consumables, materials and ammunition items from both inventory and bottomless box in one item
        // TODO use enum for category
        if (invItem.baseItem.category == parse::dsr::ItemCategory::Consumables || invItem.baseItem.category == parse::dsr::ItemCategory::Materials || invItem.baseItem.category == parse::dsr::ItemCategory::ArrowsBolts) {
            itemsById[invItem.itemId] = item;
        }
        newItems.append(item);
    }
    for (auto blbItem: charInfo->bottomlessBoxItems) {
        QStandardItem* item = nullptr;
        if (blbItem.baseItem.category == parse::dsr::ItemCategory::Consumables || blbItem.baseItem.category == parse::dsr::ItemCategory::Materials || blbItem.baseItem.category == parse::dsr::ItemCategory::ArrowsBolts) {
            auto iterItem = itemsById.find(blbItem.itemId);
            if (iterItem != itemsById.end()) item = iterItem->second;
        };
        if (item == nullptr) {
            item = createModelItem(blbItem);
            if (item == nullptr) continue;
            itemsById[blbItem.itemId] = item;
            newItems.append(item);
        }
        item->setData(blbItem.amount, ItemBottomlessBoxAmountRole);
    }

    if (!newItems.isEmpty())
        rootItem->appendRows(newItems);
}

static QPixmap getInfusionIcon(const uint16_t& infusion, const uint8_t& upgradeLevel) {
    if (!hasDSRInventoryResources()) return QPixmap{};
    switch (infusion) {
        case 100:
            return QPixmap(":/dsr_inv_images/crystal");
        case 200:
            return QPixmap(":/dsr_inv_images/lightning");
        case 300:
            return QPixmap(":/dsr_inv_images/raw");
        case 400:
            if (upgradeLevel >= 5)
                return QPixmap(":/dsr_inv_images/magic_2");
            return QPixmap(":/dsr_inv_images/magic");
        case 500:
            return QPixmap(":/dsr_inv_images/enchanted");
        case 600:
            if (upgradeLevel >= 5)
                return QPixmap(":/dsr_inv_images/divine_2");
            return QPixmap(":/dsr_inv_images/divine");
        case 700:
            return QPixmap(":/dsr_inv_images/occult");
        case 800:
            if (upgradeLevel >= 5)
                return QPixmap(":/dsr_inv_images/fire_2");
            return QPixmap(":/dsr_inv_images/fire");

        case 900:
            return QPixmap(":/dsr_inv_images/chaos");
        default:
            return QPixmap{};
    }
}

static QPixmap getItemImage(const std::string_view& image) {
    if (!hasDSRInventoryResources()) return QPixmap{};

    QString imagePath = QString::fromStdString(":/dsr_inv_images/");
    imagePath.append(QString::fromStdString(image.data()));
    imagePath.append(QString::fromStdString(""));
    return QPixmap(imagePath);
}

QStandardItem* InventoryModel::createModelItem(fssm::parse::dsr::InventoryItem& inventoryItem) {
    if (!inventoryItem.knownItem) return createUnknownItem(inventoryItem);
    // Skip fist
    if (inventoryItem.baseItem.type == 0 && inventoryItem.baseItem.id == 900000) return nullptr;
    // Skip no armor
    if (inventoryItem.baseItem.type == 268435456) {
        switch (inventoryItem.baseItem.id) {
            case 900000:
            case 901000:
            case 902000:
            case 903000:
                return nullptr;
            default:
                break;
        }
    }

    QPixmap infusionPix = getInfusionIcon(inventoryItem.infusion, inventoryItem.upgradeLevel);
    QPixmap itemImage = getItemImage(inventoryItem.baseItem.image);

    QStandardItem* item = new QStandardItem(QString::fromStdString(inventoryItem.baseItem.label.data()));
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    item->setData(inventoryItem.upgradeLevel, ItemLevelRole);
    item->setData(infusionPix, ItemInfusionIconRole);
    item->setData(inventoryItem.order, ItemOrderRole);
    item->setData(inventoryItem.durability, ItemDurabilityRole);
    item->setData(0, ItemAmountRole);
    item->setData(0, ItemBottomlessBoxAmountRole);
    item->setData(itemImage, ItemImageRole);
    item->setData(QVariant::fromValue(inventoryItem.baseItem.category), ItemCategoryRole);
    return item;
}
QStandardItem* InventoryModel::createUnknownItem(fssm::parse::dsr::InventoryItem& inventoryItem) {
    QStandardItem* item = new QStandardItem();
    QString label;
    label.push_back("NA ");
    label.push_back(QString::fromStdString(std::to_string(inventoryItem.itemType)));
    label.push_back(" ");
    label.push_back(QString::fromStdString(std::to_string(inventoryItem.itemId)));
    item->setText(label);

    item->setData(inventoryItem.upgradeLevel, ItemLevelRole);
    item->setData(inventoryItem.order, ItemOrderRole);
    item->setData(inventoryItem.durability, ItemDurabilityRole);
    item->setData(QVariant::fromValue(inventoryItem.baseItem.category), ItemCategoryRole);
    item->setData(QPixmap(":/dsr_images/unknown"), ItemImageRole);
    return item;
}


InventoryProxyModel::InventoryProxyModel(QObject *parent): QSortFilterProxyModel(parent) {
    setSortCaseSensitivity(Qt::CaseInsensitive);
    setFilterCaseSensitivity(Qt::CaseInsensitive);
}

void InventoryProxyModel::setCategory(parse::dsr::ItemCategory category) {
    if (category == m_category) return;
    beginFilterChange();
    m_category = category;
    endFilterChange(Direction::Rows);
}

bool InventoryProxyModel::lessThan(const QModelIndex &sourceLeft, const QModelIndex &sourceRight) const {
    int left = sourceLeft.data(ItemOrderRole).toInt();
    int right = sourceRight.data(ItemOrderRole).toInt();
    if (left != right) return left < right;
    return QSortFilterProxyModel::lessThan(sourceLeft, sourceRight);
}

bool InventoryProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const {
    QModelIndex index = sourceModel()->index(source_row, 0, source_parent);
    parse::dsr::ItemCategory category = index.data(ItemCategoryRole).value<parse::dsr::ItemCategory>();
    return category == m_category;
}


InventoryDelegate::InventoryDelegate(QObject* parent): QStyledItemDelegate(parent) {
    m_standPix = QPixmap(":/dsr_images/inventory_stand");
    m_inventoryBagPix = QPixmap(":/dsr_images/inventory_bag");
    m_bottomlessBoxPix = QPixmap(":/dsr_images/bottomless_box");
}

int InventoryDelegate::paintIcon(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    int textOffset = 20;
    if (!hasDSRInventoryResources()) return textOffset;

    QVariant pixmapValue = index.data(ItemImageRole);
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
        pixmap = QPixmap(":/dsr_images/unknown");
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

    QVariant infusionIconValue = index.data(ItemInfusionIconRole);
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
    QVariant levelValue = index.data(ItemLevelRole);
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

    QString amountText = QString::number(index.data(ItemAmountRole).toInt());
    QString bottomlessBoxText = QString::number(index.data(ItemBottomlessBoxAmountRole).toInt());

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

const std::unordered_map<parse::dsr::ItemCategory, QString> categoryToString = {
    {parse::dsr::ItemCategory::Consumables, "consumables"},
    {parse::dsr::ItemCategory::Materials, "materials"},
    {parse::dsr::ItemCategory::KeyItems, "key_items"},
    {parse::dsr::ItemCategory::ArrowsBolts, "ammunition"},
    {parse::dsr::ItemCategory::WeaponsShields, "weapons_shields"},
    {parse::dsr::ItemCategory::Rings, "rings"},
    {parse::dsr::ItemCategory::Armor, "armor"},
    {parse::dsr::ItemCategory::Spells, "spells"},
};

DSRInventoryCategoryButton::DSRInventoryCategoryButton(const parse::dsr::ItemCategory& category, QWidget* parent): BaseClickableFrame(parent), m_category(category) {
    setAttribute(Qt::WA_TranslucentBackground, true);
    const QString& categoryStr = categoryToString.at(category);
    m_pix = QPixmap(":/dsr_images/inventory_" + categoryStr + "");
    m_hoverPix = QPixmap(":/dsr_images/inventory_" + categoryStr + "_hover");
    m_imageLabel = new PixmapLabel(m_pix, this);
    m_imageLabel->setObjectName("dsr_category_icon");
    m_imageLabel->setAttribute(Qt::WA_TranslucentBackground, true);

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(5, 5, 5, 5);
    layout->addWidget(m_imageLabel, 1);
}
void DSRInventoryCategoryButton::setSelected(bool selected) {
    if (selected == m_isSelected) return;
    m_isSelected = selected;
    if (selected || m_isHovered) {
        m_imageLabel->setSourcePixmap(m_hoverPix);
    } else {
        m_imageLabel->setSourcePixmap(m_pix);
    }
}
void DSRInventoryCategoryButton::enterEvent(QEnterEvent *event) {
    m_isHovered = true;
    if (!m_isSelected) {
        m_imageLabel->setSourcePixmap(m_hoverPix);
    }
}
void DSRInventoryCategoryButton::leaveEvent(QEvent *event) {
    m_isHovered = false;
    if (!m_isSelected) {
        m_imageLabel->setSourcePixmap(m_pix);
    }

}
void DSRInventoryCategoryButton::onMouseRelease() {
    emit clicked(m_category);
}

CategoryButtonOverlay::CategoryButtonOverlay(QWidget* parent): QWidget(parent) {
    m_bgPix = QPixmap(":/dsr_images/inventory_overlay");

}
void CategoryButtonOverlay::paintEvent(QPaintEvent* event) {
    QPainter painter = QPainter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.drawPixmap(0, 0, m_bgPix.scaled(width(), height(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

CategoryButtons::CategoryButtons(QWidget* parent): QWidget(parent) {
    setAttribute(Qt::WA_TranslucentBackground, true);
    m_overlayWidget = new CategoryButtonOverlay(this);

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    const auto addCategory = [&](parse::dsr::ItemCategory category) {
        DSRInventoryCategoryButton* btn = new DSRInventoryCategoryButton(category, this);
        btn->stackUnder(m_overlayWidget);
        connect(btn, SIGNAL(clicked(parse::dsr::ItemCategory)), this, SLOT(setCategory(parse::dsr::ItemCategory)));
        m_categoryMapping[category] = btn;
        layout->addWidget(btn, 0);
    };

    addCategory(parse::dsr::ItemCategory::Consumables);
    addCategory(parse::dsr::ItemCategory::Materials);
    addCategory(parse::dsr::ItemCategory::KeyItems);
    addCategory(parse::dsr::ItemCategory::ArrowsBolts);
    addCategory(parse::dsr::ItemCategory::WeaponsShields);
    addCategory(parse::dsr::ItemCategory::Rings);
    addCategory(parse::dsr::ItemCategory::Armor);
    addCategory(parse::dsr::ItemCategory::Spells);

    layout->addStretch(1);

    m_overlayAnim = new QVariantAnimation();
    m_overlayAnim->setDuration(100);
    connect(m_overlayAnim, SIGNAL(valueChanged(QVariant)), this, SLOT(onAnimValueChange(QVariant)));
    connect(m_overlayAnim, SIGNAL(finished()), this, SLOT(onAnimfinished()));

    // Make sure m_category is different from 'Consumables' so it can be triggered as category change
    // TODO look if there is a better approach.
    m_category = parse::dsr::ItemCategory::Spells;
    setCategory(parse::dsr::ItemCategory::Consumables);
}

void CategoryButtons::setCategory(parse::dsr::ItemCategory category) {
    if (m_category == category) return;
    m_categoryMapping[m_category]->setSelected(false);
    m_category = category;
    m_categoryMapping[m_category]->setSelected(true);
    emit categoryChanged(category);
    if (m_overlayAnim->state() == QVariantAnimation::Running) {
        m_overlayAnim->stop();
    }
    m_overlayAnim->setStartValue(m_overlayWidget->pos());
    m_overlayAnim->setEndValue(m_categoryMapping[m_category]->pos());
    m_overlayAnim->start();
}
void CategoryButtons::showEvent(QShowEvent* event) {
    m_overlayWidget->setGeometry(m_categoryMapping[m_category]->geometry());
    m_overlayWidget->raise();
}
void CategoryButtons::resizeEvent(QResizeEvent *event) {
    m_overlayWidget->resize(m_categoryMapping[m_category]->size());
}
void CategoryButtons::onAnimValueChange(QVariant posValue) {
    m_overlayWidget->move(posValue.toPoint());
}
void CategoryButtons::onAnimfinished() {
    m_overlayWidget->move(m_overlayAnim->endValue().toPoint());
}

InventoryWidget::InventoryWidget(QWidget* parent): QWidget(parent) {
    setAttribute(Qt::WA_TranslucentBackground, true);
    m_categoryBtns = new CategoryButtons(this);

    m_view = new QListView(this);
    m_view->setObjectName("ds_list_view");
    m_view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_view->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_view->verticalScrollBar()->setSingleStep(15);
    m_view->setAttribute(Qt::WA_TranslucentBackground, true);

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

    connect(m_categoryBtns, SIGNAL(categoryChanged(parse::dsr::ItemCategory)), this, SLOT(onCategoryChange(parse::dsr::ItemCategory)));

    m_proxy->setCategory(m_categoryBtns->getCategory());
}

void InventoryWidget::setCharacter(const fssm::parse::dsr::DSRCharacterInfo* charInfo) {
    m_model->setCharacter(charInfo);
    m_proxy->sort(0, Qt::AscendingOrder);
}

void InventoryWidget::onCategoryChange(parse::dsr::ItemCategory category) {
    m_proxy->setCategory(category);
}
}
