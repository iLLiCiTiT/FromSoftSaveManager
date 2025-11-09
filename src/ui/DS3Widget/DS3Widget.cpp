#include "DS3Widget.h"

#include <iostream>
#include <qevent.h>
#include <QPainter>

#include "../ManageBackupsWidget.h"

namespace fssm::ui::ds3 {
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

    DS3CharInfoResult charsInfo = m_controller->getDs3Characters(m_saveId);
    if (!charsInfo.error.isEmpty()) {
        m_chars = std::vector<fssm::parse::ds3::DS3CharacterInfo>();
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

fssm::parse::ds3::DS3CharacterInfo* CharsListModel::getCharByIdx(const int& index) {
    for (auto& character: m_chars) {
        if (character.index == index) return &character;
    }
    return nullptr;
}
}

DS3Widget::DS3Widget(Controller* controller, const QString& saveId, QWidget* parent)
    : BaseGameWidget(controller, saveId, parent)
{
    m_bgPix = QPixmap(":/ds3_images/bg");

    QWidget* viewWrap = new QWidget(this);
    viewWrap->setAttribute(Qt::WA_TranslucentBackground, true);

    m_view = new QListView(viewWrap);
    m_view->setObjectName("ds_list_view");
    m_view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_view->setTextElideMode(Qt::ElideLeft);
    m_view->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

    ManageBackupsButtonsWidget* manageBackupsBtnsWidget = new ManageBackupsButtonsWidget(controller, viewWrap);
    manageBackupsBtnsWidget->setAttribute(Qt::WA_TranslucentBackground, true);

    m_model = new fssm::ui::ds3::CharsListModel(controller, saveId, m_view);
    m_view->setModel(m_model);

    QVBoxLayout* viewWrapLayout = new QVBoxLayout(viewWrap);
    viewWrapLayout->setContentsMargins(0, 0, 0, 0);
    viewWrapLayout->addSpacing(28);
    viewWrapLayout->addWidget(m_view, 4);
    viewWrapLayout->addWidget(manageBackupsBtnsWidget, 1);

    m_charTabs = new TabWidget(this);

    m_charInfoWidget = new fssm::ui::ds3::CharacterInfoWidget(m_charTabs);
    m_inventoryWidget = new fssm::ui::ds3::InventoryWidget(m_charTabs);

    m_charTabs->addTab(
        "Character Info",
        m_charInfoWidget
    );
    m_charTabs->addTab(
        "Inventory",
        m_inventoryWidget
    );

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->addWidget(viewWrap, 0);
    layout->addWidget(m_charTabs, 1);

    connect(m_view->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this, SLOT(onSelectionChange(const QItemSelection&, const QItemSelection&)));
    connect(m_model, SIGNAL(refreshed()), this, SLOT(onRefresh()));
    connect(manageBackupsBtnsWidget, SIGNAL(showBackupsRequested()), this, SIGNAL(showBackupsRequested()));
}

void DS3Widget::refresh() {
    m_model->refresh();
}

void DS3Widget::paintEvent(QPaintEvent* event) {
    QPainter painter = QPainter(this);
    painter.setClipRect(event->rect());
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(6, 5, 7));
    const QRect targetRect = rect();
    painter.drawRect(targetRect);
    painter.drawPixmap(0, 0, m_bgPix.scaled(size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
}

void DS3Widget::onRefresh() {
    QItemSelectionModel* selModel = m_view->selectionModel();
    for (auto& index: selModel->selectedIndexes()) {
        QVariant charId = index.data(fssm::ui::ds3::CHAR_ID_ROLE);
        if (!charId.isValid() || charId.isNull()) continue;
        fssm::parse::ds3::DS3CharacterInfo* charInfo = m_model->getCharByIdx(charId.toInt());
        m_charInfoWidget->setCharacter(charInfo);
        m_inventoryWidget->setCharacter(charInfo);
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

void DS3Widget::onSelectionChange(const QItemSelection &selected, const QItemSelection &deselected) {
    for (auto& index: selected.indexes()) {
        QVariant charId = index.data(fssm::ui::ds3::CHAR_ID_ROLE);
        if (!charId.isValid() || charId.isNull()) continue;
        const fssm::parse::ds3::DS3CharacterInfo* charInfo = m_model->getCharByIdx(charId.toInt());

        m_charInfoWidget->setCharacter(charInfo);
        m_inventoryWidget->setCharacter(charInfo);
        return;
    }
    m_charInfoWidget->setCharacter(nullptr);
    m_inventoryWidget->setCharacter(nullptr);
}
