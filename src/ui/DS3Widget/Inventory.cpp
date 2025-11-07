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

namespace {
    enum class FlaskIconType {
        Empty,
        Full,
        Half,
        Quarter,
    };
    constexpr FlaskIconType mapFlaskAmounts(const int& max_amount, const int& amount) {
        if (amount == 0) return FlaskIconType::Empty;
        if (max_amount == 1 || max_amount == 2) return FlaskIconType::Full;
        int full = 0;
        int half = 0;

        switch (max_amount) {
            case 3:
            case 4:
                full = 2;
                half = 1;
                break;
            case 5:
                full = 3;
                half = 1;
                break;
            case 6:
            case 7:
                full = 4;
                half = 2;
                break;
            case 8:
                full = 5;
                half = 3;
                break;
            case 9:
                full = 6;
                half = 3;
                break;
            case 10:
                full = 7;
                half = 3;
                break;
            case 11:
                full = 7;
                half = 4;
                break;
            case 12:
                full = 8;
                half = 4;
                break;
            case 13:
            case 14:
                full = 9;
                half = 5;
                break;
            default:
                full = 10;
                half = 5;
                break;
        }

        if (amount > full) return FlaskIconType::Full;
        if (amount > half) return FlaskIconType::Half;
        return FlaskIconType::Quarter;
    }

    std::string_view getInfusionName(const fssm::parse::ds3::Infusion& infusion) {
        if (!hasDS3InventoryResources()) return "";
        switch (infusion) {
            case fssm::parse::ds3::Infusion::Heavy:
                return "heavy";
            case fssm::parse::ds3::Infusion::Sharp:
                return "sharp";
            case fssm::parse::ds3::Infusion::Refined:
                return "refined";
            case fssm::parse::ds3::Infusion::Simple:
                return "simple";
            case fssm::parse::ds3::Infusion::Crystal:
                return "crystal";
            case fssm::parse::ds3::Infusion::Fire:
                return "fire";
            case fssm::parse::ds3::Infusion::Chaos:
                return "chaos";
            case fssm::parse::ds3::Infusion::Lighting:
                return "lighting";
            case fssm::parse::ds3::Infusion::Deep:
                return "deep";
            case fssm::parse::ds3::Infusion::Dark:
                return "dark";
            case fssm::parse::ds3::Infusion::Poison:
                return "poison";
            case fssm::parse::ds3::Infusion::Blood:
                return "blood";
            case fssm::parse::ds3::Infusion::Raw:
                return "raw";
            case fssm::parse::ds3::Infusion::Blessed:
                return "blessed";
            case fssm::parse::ds3::Infusion::Hollow:
                return "hollow";
            default:
                return "";
        }
    }

    constexpr bool isMergable(fssm::parse::ds3::InventoryItem& invItem) {
        switch (invItem.baseItem.category) {
            case fssm::parse::ds3::ItemCategory::Tools:
            case fssm::parse::ds3::ItemCategory::Materials:
            case fssm::parse::ds3::ItemCategory::Spells:
            case fssm::parse::ds3::ItemCategory::ArrowsBolts:
                return true;
            default:
                return false;
        }
    }
}

namespace fssm::ui::ds3 {

InventoryModel::InventoryModel(QObject* parent): QStandardItemModel(parent) {}

void InventoryModel::setCharacter(const fssm::parse::ds3::DS3CharacterInfo* charInfo) {
    QStandardItem* rootItem = invisibleRootItem();
    rootItem->removeRows(0, rootItem->rowCount());
    if (charInfo == nullptr) return;

    std::unordered_map<uint32_t, QStandardItem*> itemsById;
    QList<QStandardItem*> newItems;
    std::vector<fssm::parse::ds3::InventoryItem*> inventoryItems;
    for (auto invItem: charInfo->inventoryItems) {
        // TODO find out if '1073741918' is in key items or in inventory items
        if (invItem.itemId == 1073741918) continue;
        QStandardItem* item = createModelItem(charInfo, invItem);
        item->setData(invItem.amount, ITEM_AMOUNT_ROLE);
        newItems.push_back(item);
        if (isMergable(invItem)) itemsById[invItem.itemId] = item;
    }

    for (auto invItem: charInfo->keyItems) {
        if (invItem.itemId == 1073741918) continue;
        QStandardItem* item = createModelItem(charInfo, invItem);
        item->setData(invItem.amount, ITEM_AMOUNT_ROLE);
        newItems.push_back(item);
        if (isMergable(invItem)) itemsById[invItem.itemId] = item;
    }

    for (auto invItem: charInfo->storageBoxItems) {
        QStandardItem* item;
        auto it = itemsById.find(invItem.itemId);
        if (it != itemsById.end()) {
            item = it->second;
        } else {
            item = createModelItem(charInfo, invItem);
            newItems.push_back(item);
        }
        item->setData(invItem.amount, ITEM_STORAGE_BOX_AMOUNT_ROLE);
    }
    if (!newItems.isEmpty())
        rootItem->appendRows(newItems);
}

static QPixmap getItemImage(const std::string_view& image) {
    if (!hasDS3InventoryResources()) return QPixmap{};

    QString imagePath = QString::fromStdString(":/ds3_inv_images/");
    imagePath.append(QString::fromStdString(image.data()));
    return QPixmap(imagePath);
}

QStandardItem* InventoryModel::createModelItem(const parse::ds3::DS3CharacterInfo* charInfo, parse::ds3::InventoryItem& invItem) {
    if (invItem.baseItem.id == 0) return createUnknownItem(invItem);

    std::string_view infusionName = getInfusionName(invItem.infusion);
    QPixmap infusionImage = getItemImage(infusionName);

    QString label = invItem.baseItem.label.data();
    if (!infusionName.empty()) {
        std::string tmpName = infusionName.data();
        tmpName[0] = toupper(tmpName[0]);
        QString infusionLabel = QString::fromStdString(invItem.baseItem.infusion_label.data());
        if (infusionLabel.isEmpty()) {
            label.append(" ");
            label.append(tmpName);
        } else {
            label = infusionLabel.replace("{infusion}", QString::fromStdString(tmpName));
        }
    }
    if (invItem.upgradeLevel > 0) {
        label.append(" + ");
        label.append(QString::number(invItem.upgradeLevel));
    }

    std::string_view imageName = invItem.baseItem.image;
    if (1073741974 <= invItem.itemId && invItem.itemId <= 1073741995) {
        switch (mapFlaskAmounts(charInfo->estusMax, invItem.amount)) {
            case FlaskIconType::Empty:
                imageName = "estus_flask_empty";
                break;
            case FlaskIconType::Half:
                imageName = "estus_flask_half";
                break;
            case FlaskIconType::Quarter:
                imageName = "estus_flask_quater";
                break;
        }

    } else if (1073742014 <= invItem.itemId && invItem.itemId <= 1073742033) {
        switch (mapFlaskAmounts(charInfo->ashenEstusMax, invItem.amount)) {
            case FlaskIconType::Empty:
                imageName = "ashen_estus_flask_empty";
                break;
            case FlaskIconType::Half:
                imageName = "ashen_estus_flask_half";
                break;
            case FlaskIconType::Quarter:
                imageName = "ashen_estus_flask_quater";
                break;
        }
    }
    QPixmap itemImage = getItemImage(imageName);

    QStandardItem* item = new QStandardItem(label);
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    item->setData(infusionImage, ITEM_INFUSION_ICON_ROLE);
    item->setData(invItem.baseItem.order, ITEM_ORDER_ROLE);
    item->setData(0, ITEM_AMOUNT_ROLE);
    item->setData(0, ITEM_STORAGE_BOX_AMOUNT_ROLE);
    item->setData(itemImage, ITEM_IMAGE_ROLE);
    item->setData(QVariant::fromValue(invItem.baseItem.category), ITEM_CATEGORY_ROLE);
    return item;
}

QStandardItem* InventoryModel::createUnknownItem(fssm::parse::ds3::InventoryItem& inventoryItem) {
    QStandardItem* item = new QStandardItem();
    QString label;
    label.push_back("NA ");
    label.push_back(QString::fromStdString(std::to_string(inventoryItem.itemId)));
    if (inventoryItem.upgradeLevel > 0) {
        label.push_back(" + ");
        label.push_back(QString::number(inventoryItem.upgradeLevel));
    }
    item->setText(label);

    item->setData(0, ITEM_STORAGE_BOX_AMOUNT_ROLE);
    item->setData(-1, ITEM_ORDER_ROLE);
    item->setData(inventoryItem.amount, ITEM_AMOUNT_ROLE);
    item->setData(QVariant::fromValue(inventoryItem.baseItem.category), ITEM_CATEGORY_ROLE);
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
    m_standPix = QPixmap(":/ds3_images/dish");
    m_inventoryBagPix = QPixmap(":/ds3_images/menu_invetory");
    m_bottomlessBoxPix = QPixmap(":/ds3_images/menu_storage_box");
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
        pixmap = QPixmap(":/ds3_images/test_data");
    int imgSize = option.rect.height();
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

    int stand_height = 21;
    QRect standRect = QRect(
        iconRect.left() + 10,
        iconRect.bottom() - stand_height,
        iconRect.width() - 20,
        stand_height
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
            iconRect.right() - infusionIcon.width(),
            (iconRect.bottom() - infusionIcon.height()) - 5,
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
    QString bottomlessBoxText = QString::number(index.data(ITEM_STORAGE_BOX_AMOUNT_ROLE).toInt());

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
    // center
    const int x = (width() - scaled.width()) / 2;
    const int y = (height() - scaled.height()) / 2;
    painter.drawPixmap(x, y, scaled);
}

CategoryButtons::CategoryButtons(QWidget* parent): QWidget(parent) {
    setAttribute(Qt::WA_TranslucentBackground, true);

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    const auto addCategory = [&](const fssm::parse::ds3::ItemCategory& category) {
        DS3InventoryCategoryButton* btn = new DS3InventoryCategoryButton(category, this);
        connect(btn, SIGNAL(clicked(parse::ds3::ItemCategory)), this, SLOT(setCategory(parse::ds3::ItemCategory)));
        if (m_category == category) btn->setSelected(true);
        m_categoryMapping[category] = btn;
        layout->addWidget(btn, 0);
    };

    addCategory(parse::ds3::ItemCategory::Tools);
    addCategory(parse::ds3::ItemCategory::Materials);
    addCategory(parse::ds3::ItemCategory::KeyItems);
    addCategory(parse::ds3::ItemCategory::Spells);
    addCategory(parse::ds3::ItemCategory::MeleeWeapons);
    addCategory(parse::ds3::ItemCategory::RangedWeapons);
    addCategory(parse::ds3::ItemCategory::Catalysts);
    addCategory(parse::ds3::ItemCategory::Shields);
    addCategory(parse::ds3::ItemCategory::HeadArmor);
    addCategory(parse::ds3::ItemCategory::ChestArmor);
    addCategory(parse::ds3::ItemCategory::HandsArmor);
    addCategory(parse::ds3::ItemCategory::LegsArmor);
    addCategory(parse::ds3::ItemCategory::ArrowsBolts);
    addCategory(parse::ds3::ItemCategory::Rings);
    addCategory(parse::ds3::ItemCategory::CovenantItem);
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
