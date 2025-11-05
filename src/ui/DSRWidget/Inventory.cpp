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

InventoryModel::InventoryModel(QObject* parent): QStandardItemModel(parent) {};


void InventoryModel::setCharacter(const fssm::parse::dsr::DSRCharacterInfo* charInfo) {
    QStandardItem* rootItem = invisibleRootItem();
    rootItem->removeRows(0, rootItem->rowCount());
    if (charInfo == nullptr) return;

    std::unordered_map<uint32_t, QStandardItem*> itemsById;
    QList<QStandardItem*> newItems;
    for (auto invItem: charInfo->inventoryItems) {
        QStandardItem* item = createModelItem(invItem);
        if (item == nullptr) continue;

        item->setData(invItem.amount, ITEM_AMOUNT_ROLE);
        // Show consumables, materials and ammunition items from both inventory and bottomless box in one item
        // TODO use enum for category
        if (invItem.baseItem.category == "consumables" || invItem.baseItem.category == "materials" || invItem.baseItem.category == "ammunition") {
            itemsById[invItem.itemId] = item;
        }
        newItems.append(item);
    }
    for (auto blbItem: charInfo->botomlessBoxItems) {
        QStandardItem* item = nullptr;
        if (blbItem.baseItem.category == "consumables" || blbItem.baseItem.category == "materials" || blbItem.baseItem.category == "ammunition") {
            auto iterItem = itemsById.find(blbItem.itemId);
            if (iterItem != itemsById.end()) item = iterItem->second;
        };
        if (item == nullptr) {
            item = createModelItem(blbItem);
            if (item == nullptr) continue;
            itemsById[blbItem.itemId] = item;
            newItems.append(item);
        }
        item->setData(blbItem.amount, ITEM_BOTOMLESS_BOX_AMOUNT_ROLE);
    }

    if (!newItems.isEmpty())
        rootItem->appendRows(newItems);
};

static QPixmap getInfusionIcon(const uint16_t& infusion, const uint8_t& upgradeLevel) {
    if (!hasDSRInventoryResources()) return QPixmap{};
    switch (infusion) {
        case 100:
            return QPixmap(":/dsr_inv_images/crystal.png");
        case 200:
            return QPixmap(":/dsr_inv_images/lightning.png");
        case 300:
            return QPixmap(":/dsr_inv_images/raw.png");
        case 400:
            if (upgradeLevel >= 5)
                return QPixmap(":/dsr_inv_images/magic_2.png");
            return QPixmap(":/dsr_inv_images/magic.png");
        case 500:
            return QPixmap(":/dsr_inv_images/enchanted.png");
        case 600:
            if (upgradeLevel >= 5)
                return QPixmap(":/dsr_inv_images/divine_2.png");
            return QPixmap(":/dsr_inv_images/divine.png");
        case 700:
            return QPixmap(":/dsr_inv_images/occult.png");
        case 800:
            if (upgradeLevel >= 5)
                return QPixmap(":/dsr_inv_images/fire_2.png");
            return QPixmap(":/dsr_inv_images/fire.png");

        case 900:
            return QPixmap(":/dsr_inv_images/chaos.png");
        default:
            return QPixmap{};
    }
}

static QPixmap getItemImage(const std::string_view& image) {
    if (!hasDSRInventoryResources()) return QPixmap{};

    QString imagePath = QString::fromStdString(":/dsr_inv_images/");
    imagePath.append(QString::fromStdString(image.data()));
    imagePath.append(QString::fromStdString(".png"));
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

    item->setData(inventoryItem.upgradeLevel, ITEM_LEVEL_ROLE);
    item->setData(infusionPix, ITEM_INFUSION_ICON_ROLE);
    item->setData(inventoryItem.order, ITEM_ORDER_ROLE);
    item->setData(inventoryItem.durability, ITEM_DURABILITY_ROLE);
    item->setData(0, ITEM_AMOUNT_ROLE);
    item->setData(0, ITEM_BOTOMLESS_BOX_AMOUNT_ROLE);
    item->setData(itemImage, ITEM_IMAGE_ROLE);
    item->setData(QString::fromStdString(inventoryItem.baseItem.category.data()), ITEM_CATEGORY_ROLE);
    return item;
};
QStandardItem* InventoryModel::createUnknownItem(fssm::parse::dsr::InventoryItem& inventoryItem) {
    QStandardItem* item = new QStandardItem();
    QString label;
    label.push_back("NA ");
    label.push_back(QString::fromStdString(std::to_string(inventoryItem.itemType)));
    label.push_back(" ");
    label.push_back(QString::fromStdString(std::to_string(inventoryItem.itemId)));
    item->setText(label);

    item->setData(inventoryItem.upgradeLevel, ITEM_LEVEL_ROLE);
    item->setData(inventoryItem.order, ITEM_ORDER_ROLE);
    item->setData(inventoryItem.durability, ITEM_DURABILITY_ROLE);
    item->setData(QVariant(QString::fromStdString(inventoryItem.baseItem.category.data())), ITEM_CATEGORY_ROLE);
    item->setData(QPixmap(":/dsr_images/unknown.png"), ITEM_IMAGE_ROLE);
    return item;
};


InventoryProxyModel::InventoryProxyModel(QObject *parent): QSortFilterProxyModel(parent) {
    setSortCaseSensitivity(Qt::CaseInsensitive);
    setFilterCaseSensitivity(Qt::CaseInsensitive);
};

void InventoryProxyModel::setCategory(QString category) {
    if (category == m_category) return;
    beginFilterChange();
    m_category = category;
    endFilterChange(Direction::Rows);
};

bool InventoryProxyModel::lessThan(const QModelIndex &sourceLeft, const QModelIndex &sourceRight) const {
    int left = sourceLeft.data(ITEM_ORDER_ROLE).toInt();
    int right = sourceRight.data(ITEM_ORDER_ROLE).toInt();
    if (left != right) return left < right;
    return QSortFilterProxyModel::lessThan(sourceLeft, sourceRight);
};

bool InventoryProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const {
    if (m_category.isEmpty()) return true;
    QModelIndex index = sourceModel()->index(source_row, 0, source_parent);
    QString category = index.data(ITEM_CATEGORY_ROLE).toString();
    return category == m_category;
};


InventoryDelegate::InventoryDelegate(QObject* parent): QStyledItemDelegate(parent) {
    m_standPix = QPixmap(":/dsr_images/inventory_stand.png");
    m_inventoryBagPix = QPixmap(":/dsr_images/inventory_bag.png");
    m_bottomlessBoxPix = QPixmap(":/dsr_images/bottomless_box.png");
}

int InventoryDelegate::paintIcon(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    int textOffset = 20;
    if (!hasDSRInventoryResources()) return textOffset;

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
        pixmap = QPixmap(":/dsr_images/unknown.png");
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
};

QSize InventoryDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const {
    return QSize(260, 80);
};


DSRInventoryCategoryButton::DSRInventoryCategoryButton(const QString& category, QWidget* parent): BaseClickableFrame(parent), m_category(category) {
    m_pix = QPixmap(":/dsr_images/inventory_" + category + ".png");
    m_hoverPix = QPixmap(":/dsr_images/inventory_" + category + "_hover.png");
    m_imageLabel = new PixmapLabel(m_pix, this);
    m_imageLabel->setObjectName("dsr_category_icon");

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(5, 5, 5, 5);
    layout->addWidget(m_imageLabel, 1);
};
void DSRInventoryCategoryButton::setSelected(bool selected) {
    if (selected == m_isSelected) return;
    m_isSelected = selected;
    if (selected || m_isHovered) {
        m_imageLabel->setSourcePixmap(m_hoverPix);
    } else {
        m_imageLabel->setSourcePixmap(m_pix);
    }
};
void DSRInventoryCategoryButton::enterEvent(QEnterEvent *event) {
    m_isHovered = true;
    if (!m_isSelected) {
        m_imageLabel->setSourcePixmap(m_hoverPix);
    }
};
void DSRInventoryCategoryButton::leaveEvent(QEvent *event) {
    m_isHovered = false;
    if (!m_isSelected) {
        m_imageLabel->setSourcePixmap(m_pix);
    }

};
void DSRInventoryCategoryButton::onMouseRelease() {
    emit clicked(m_category);
};

CategoryButtonOverlay::CategoryButtonOverlay(QWidget* parent): QWidget(parent) {
    m_bgPix = QPixmap(":/dsr_images/inventory_overlay.png");

};
void CategoryButtonOverlay::paintEvent(QPaintEvent* event) {
    QPainter painter = QPainter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.drawPixmap(0, 0, m_bgPix.scaled(width(), height(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
};

CategoryButtons::CategoryButtons(QWidget* parent): QWidget(parent) {
    m_overlayWidget = new CategoryButtonOverlay(this);

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    for (auto category : std::initializer_list<QString> {
        "consumables",
        "materials",
        "key_items",
        "spells",
        "weapons_shields",
        "ammunition",
        "armor",
        "rings"
    }) {
        DSRInventoryCategoryButton* btn = new DSRInventoryCategoryButton(category, this);
        btn->stackUnder(m_overlayWidget);
        connect(btn, SIGNAL(clicked(QString)), this, SLOT(setCategory(QString)));
        if (m_category.isEmpty()) {
            m_category = category;
            btn->setSelected(true);
        };
        m_categoryMapping[category] = btn;
        layout->addWidget(btn, 0);
    }
    layout->addStretch(1);

    m_overlayAnim = new QVariantAnimation();
    m_overlayAnim->setDuration(100);
    connect(m_overlayAnim, SIGNAL(valueChanged(QVariant)), this, SLOT(onAnimValueChange(QVariant)));
    connect(m_overlayAnim, SIGNAL(finished()), this, SLOT(onAnimfinished()));
};
QString CategoryButtons::getCategory() {
    return m_category;
};
void CategoryButtons::setCategory(QString category) {
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

};
void CategoryButtons::showEvent(QShowEvent* event) {
    m_overlayWidget->setGeometry(m_categoryMapping[m_category]->geometry());
    m_overlayWidget->raise();
};
void CategoryButtons::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    m_overlayWidget->resize(m_categoryMapping[m_category]->size());
};
void CategoryButtons::onAnimValueChange(QVariant posValue) {
    m_overlayWidget->move(posValue.toPoint());
};
void CategoryButtons::onAnimfinished() {
    m_overlayWidget->move(m_overlayAnim->endValue().toPoint());
};

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

    connect(m_categoryBtns, SIGNAL(categoryChanged(QString)), this, SLOT(onCategoryChange(QString)));

    m_proxy->setCategory(m_categoryBtns->getCategory());
};

void InventoryWidget::setCharacter(const fssm::parse::dsr::DSRCharacterInfo* charInfo) {
    m_model->setCharacter(charInfo);
    m_proxy->sort(0, Qt::AscendingOrder);
};

void InventoryWidget::onCategoryChange(QString category) {
    m_proxy->setCategory(category);
};
