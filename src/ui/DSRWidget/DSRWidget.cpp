#include "DSRWidget.h"

#include <iostream>
#include <qevent.h>
#include <QPainter>


CharsListModel::CharsListModel(Controller* controller, const QString& saveId, QObject* parent)
    : QStandardItemModel(parent),
    m_saveId(saveId),
    m_controller(controller)
{
    setColumnCount(1);
    QStandardItem* root = invisibleRootItem();
    for (int i = 0; i < 10; ++i) {
        QStandardItem* item = new QStandardItem("< Empty >");
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        item->setData(QVariant(), CHAR_NAME_ROLE);
        item->setData(QVariant(i), CHAR_ID_ROLE);
        root->appendRow(item);
    }
}

void CharsListModel::refresh() {
    // TODO capture error and use it in first item (NoFlags)
    QStandardItem* root = invisibleRootItem();

    DSRCharInfoResult charsInfo = m_controller->getDsrCharacters(m_saveId);
    if (!charsInfo.error.isEmpty()) {
        m_chars = std::vector<fsm::parse::DSRCharacterInfo>();
        QStandardItem* item = root->child(0);
        item->setText(charsInfo.error);
        for (int i = 1; i < root->rowCount(); ++i) {
            QStandardItem* item = root->child(i);
            item->setText("");
        }
        item->setData(QVariant(), CHAR_NAME_ROLE);
        emit refreshed();
        return;
    }
    m_chars = charsInfo.characters;
    for (int i = 0; i < root->rowCount(); ++i) {
        QStandardItem* item = root->child(i);
        bool found = false;
        for (auto& character: m_chars) {
            if (i == character.index) {
                item->setText(QString::fromStdU16String(character.name));
                item->setData(QString::fromStdU16String(character.name), CHAR_NAME_ROLE);
                found = true;
                break;
            }
        }
        if (found) continue;
        item->setText("< Empty >");
        item->setData(QVariant(), CHAR_NAME_ROLE);
    }
    emit refreshed();
};

fsm::parse::DSRCharacterInfo* CharsListModel::getCharByIdx(const int& index) {
    for (auto& character: m_chars) {
        if (character.index == index) return &character;
    }
    return nullptr;
}


DSRWidget::DSRWidget(Controller* controller, const QString& saveId, QWidget* parent)
    : BaseGameWidget(controller, saveId, parent)
{
    QWidget* viewWrap = new QWidget(this);

    m_view = new QListView(viewWrap);
    m_view->setObjectName("ds_list_view");
    m_view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_view->setTextElideMode(Qt::ElideLeft);
    m_view->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

    // manage_saves_widget = ManageSavesWidget(controller, view_wrap)

    m_model = new CharsListModel(controller, saveId, m_view);
    m_view->setModel(m_model);

    QVBoxLayout* viewWrapLayout = new QVBoxLayout(viewWrap);
    viewWrapLayout->setContentsMargins(0, 0, 0, 0);
    viewWrapLayout->addSpacing(28);
    viewWrapLayout->addWidget(m_view, 4);
    // viewWrapLayout->addWidget(manage_saves_widget, 1);

    m_charTabs = new TabWidget(this);

    m_covenantsWidget = new CovenantsWidget(m_charTabs);
    m_charInfoWidget = new CharacterInfoWidget(m_charTabs);
    m_inventoryWidget = new InventoryWidget(m_charTabs);
    // m_charEquipWidget = new QWidget(m_charTabs);

    m_charTabs->addTab(
        "Character Info",
        m_charInfoWidget
    );
    m_charTabs->addTab(
        "Inventory",
        m_inventoryWidget
    );
    m_charTabs->addTab(
        "Covenants",
        m_covenantsWidget
    );
    // m_charTabs->addTab(
    //     "Equipment",
    //     m_charEquipWidget
    // );

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->addWidget(viewWrap, 0);
    layout->addWidget(m_charTabs, 1);

    connect(m_view->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this, SLOT(onSelectionChange(const QItemSelection&, const QItemSelection&)));
    connect(m_model, SIGNAL(refreshed()), this, SLOT(onRefresh()));
};

void DSRWidget::refresh() {
    m_model->refresh();
};

void DSRWidget::paintEvent(QPaintEvent* event) {
    QPainter painter = QPainter(this);
    painter.setClipRect(event->rect());
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(6, 5, 7));
    const QRect targetRect = rect();
    painter.drawRect(targetRect);
    QPixmap pix = QPixmap(":/dsr_images/bg.png");
    QPixmap scaled = pix.scaled(size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    // center vertically
    const int y = (height() - scaled.height()) / 2;
    painter.drawPixmap(0, y, scaled);
    // --- Manual approach ---
    // const double sAspect = double(pix.width()) / pix.height();
    // const double tAspect = double(targetRect.width()) / targetRect.height();
    // int posY = 0;
    // int srcWidth = pix.width();
    // int srcHeight = pix.height();
    // if (tAspect > sAspect) {
    //     // Crop vertically in source and offset y position
    //     srcHeight = int(pix.width() / tAspect);
    //     posY = (pix.height() - srcHeight) / 2;
    // } else {
    //     // Crop horizontally in source
    //     srcWidth = int(pix.height() * tAspect);
    // }
    // QRect src = QRect(0, posY, srcWidth, srcHeight);
    // painter.drawPixmap(targetRect, pix, src);
};

void DSRWidget::onRefresh() {
    QItemSelectionModel* selModel = m_view->selectionModel();
    for (auto& index: selModel->selectedIndexes()) {
        QVariant charId = index.data(CHAR_ID_ROLE);
        if (!charId.isValid() || charId.isNull()) continue;
        fsm::parse::DSRCharacterInfo* charInfo = m_model->getCharByIdx(charId.toInt());
        m_charInfoWidget->setCharacter(charInfo);
        m_inventoryWidget->setCharacter(charInfo);
        m_covenantsWidget->setCharacter(charInfo);
        return;
    }
    for (int row = 0; row < m_model->rowCount(); ++row) {
        QModelIndex index = m_model->index(row, 0);
        if (index.flags() & Qt::ItemIsSelectable) {
            selModel->setCurrentIndex(index, QItemSelectionModel::ClearAndSelect);
            return;
        }
    }
    selModel->clear();
};

void DSRWidget::onSelectionChange(const QItemSelection &selected, const QItemSelection &deselected) {
    for (auto& index: selected.indexes()) {
        QVariant charId = index.data(CHAR_ID_ROLE);
        if (!charId.isValid() || charId.isNull()) continue;
        const fsm::parse::DSRCharacterInfo* charInfo = m_model->getCharByIdx(charId.toInt());

        m_charInfoWidget->setCharacter(charInfo);
        m_inventoryWidget->setCharacter(charInfo);
        m_covenantsWidget->setCharacter(charInfo);
        return;
    }
    m_charInfoWidget->setCharacter(nullptr);
    m_inventoryWidget->setCharacter(nullptr);
    m_covenantsWidget->setCharacter(nullptr);
};
