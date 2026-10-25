#include "DS2Widget.h"

#include <QPainter>
#include <QVBoxLayout>
#include <qevent.h>

#include "../ManageBackupsWidget.h"
#include "../../parse/DS2/Areas.h"

namespace fssm::ui::ds2 {
CharsListModel::CharsListModel(Controller* controller, const QString& saveId, QObject* parent)
    : QStandardItemModel(parent),
    m_saveId(saveId),
    m_controller(controller)
{
    setColumnCount(1);
    QStandardItem* root = invisibleRootItem();
    for (int i = 0; i < 10; ++i) {
        QStandardItem* item = new QStandardItem("");
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        item->setData(QVariant(), CharNameRole);
        item->setData(QVariant(i), CharIdRole);
        root->appendRow(item);
    }
}

void CharsListModel::refresh() {
    QStandardItem* root = invisibleRootItem();

    DS2CharInfoResult charsInfo = m_controller->getDs2Characters(m_saveId);
    if (!charsInfo.error.isEmpty()) {
        m_chars.clear();
        for (int i = 0; i < root->rowCount(); ++i) {
            root->child(i)->setText(i == 0 ? charsInfo.error : "");
            root->child(i)->setData(QVariant(), CharNameRole);
        }
        emit refreshed();
        return;
    }
    m_chars = charsInfo.characters;
    for (int i = 0; i < root->rowCount(); ++i) {
        QStandardItem* item = root->child(i);
        const fssm::parse::ds2::DS2CharacterInfo* character = getCharByIdx(i);
        if (character == nullptr) {
            item->setText("");
            item->setData(QVariant(), CharNameRole);
            continue;
        }
        const QString name = QString::fromStdU16String(character->name);
        const auto area = fssm::parse::ds2::findAreaName(character->mapId);
        item->setText(name);
        item->setData(name, CharNameRole);
        item->setData(character->level, CharLevelRole);
        item->setData(QString::fromUtf8(area.data(), int(area.size())), CharAreaRole);
        item->setData(character->playTime, CharPlayTimeRole);
    }
    emit refreshed();
}

const fssm::parse::ds2::DS2CharacterInfo* CharsListModel::getCharByIdx(const int& index) const {
    for (auto& character: m_chars) {
        if (character.index == index) return &character;
    }
    return nullptr;
}
}

DS2Widget::DS2Widget(Controller* controller, const QString& saveId, QWidget* parent)
    : BaseGameWidget(controller, saveId, parent)
{
    setAttribute(Qt::WA_TranslucentBackground, true);
    m_bgPix = QPixmap(":/ds2_images/bg");

    QWidget* viewWrap = new QWidget(this);
    viewWrap->setAttribute(Qt::WA_TranslucentBackground, true);

    m_view = new QListView();
    m_view->setObjectName("ds2_list_view");
    m_view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_view->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_view->setMouseTracking(true);
    m_view->setFrameShape(QFrame::NoFrame);
    m_view->setItemDelegate(new fssm::ui::ds2::CharacterDelegate(m_view));
    auto* dataListFrame = new fssm::ui::ds2::DataListFrame(m_view, viewWrap);

    ManageBackupsButtonsWidget* manageBackupsBtnsWidget = new ManageBackupsButtonsWidget(controller, viewWrap);
    manageBackupsBtnsWidget->setAttribute(Qt::WA_TranslucentBackground, true);

    m_model = new fssm::ui::ds2::CharsListModel(controller, saveId, m_view);
    m_view->setModel(m_model);

    QVBoxLayout* viewWrapLayout = new QVBoxLayout(viewWrap);
    viewWrapLayout->setContentsMargins(0, 0, 0, 0);
    // Lines the Data List frame up with the pages below the header tabs.
    viewWrapLayout->addSpacing(fssm::ui::ds2::HeaderTabs::HEADER_HEIGHT);
    viewWrapLayout->addWidget(dataListFrame, 1);
    viewWrapLayout->addWidget(manageBackupsBtnsWidget, 0);

    using fssm::ui::ds2::HeaderTabs;
    m_charTabs = new HeaderTabs(this);
    m_statusWidget = new fssm::ui::ds2::PlayerStatusWidget(m_charTabs);
    m_inventoryWidget = new fssm::ui::ds2::InventoryWidget(m_charTabs);
    m_worldWidget = new fssm::ui::ds2::WorldProgressWidget(fssm::ui::ds2::WorldPage::Progress, m_charTabs);
    m_bonfiresWidget = new fssm::ui::ds2::WorldProgressWidget(fssm::ui::ds2::WorldPage::Bonfires, m_charTabs);
    m_charTabs->addTab("Player Status", HeaderTabs::Medallion::Status, m_statusWidget);
    m_charTabs->addTab("Inventory", HeaderTabs::Medallion::Inventory, m_inventoryWidget);
    m_charTabs->addTab("World Progress", HeaderTabs::Medallion::Scroll, m_worldWidget);
    // The game's bonfire is a coiled sword in the ashes.
    m_charTabs->addTab("Bonfires", HeaderTabs::Medallion::Equipment, m_bonfiresWidget);

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(20, 8, 20, 20);
    layout->addWidget(viewWrap, 0);
    layout->addWidget(m_charTabs, 1);

    connect(m_view->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
            this, SLOT(onSelectionChange(const QItemSelection&, const QItemSelection&)));
    connect(m_model, SIGNAL(refreshed()), this, SLOT(onRefresh()));
    connect(manageBackupsBtnsWidget, SIGNAL(showBackupsRequested()), this, SIGNAL(showBackupsRequested()));
}

void DS2Widget::refresh() {
    m_model->refresh();
}

void DS2Widget::showCharacter(const QModelIndex& index) {
    const QVariant charId = index.data(fssm::ui::ds2::CharIdRole);
    const fssm::parse::ds2::DS2CharacterInfo* charInfo = nullptr;
    if (charId.isValid() && !charId.isNull()) charInfo = m_model->getCharByIdx(charId.toInt());
    m_statusWidget->setCharacter(charInfo);
    m_inventoryWidget->setCharacter(charInfo);
    m_worldWidget->setCharacter(charInfo);
    m_bonfiresWidget->setCharacter(charInfo);
}

void DS2Widget::onRefresh() {
    // Like the game's list, only slots with a character are shown (or the error in row 0).
    for (int row = 0; row < m_model->rowCount(); ++row) {
        const QModelIndex index = m_model->index(row, 0);
        const bool hasCharacter = index.data(fssm::ui::ds2::CharNameRole).isValid();
        const bool isError = row == 0 && !hasCharacter && !index.data(Qt::DisplayRole).toString().isEmpty();
        m_view->setRowHidden(row, !hasCharacter && !isError);
    }

    QItemSelectionModel* selModel = m_view->selectionModel();
    const auto selected = selModel->selectedIndexes();
    if (!selected.isEmpty()) {
        showCharacter(selected.first());
        return;
    }
    for (int row = 0; row < m_model->rowCount(); ++row) {
        QModelIndex index = m_model->index(row, 0);
        if (index.data(fssm::ui::ds2::CharNameRole).isValid()) {
            selModel->setCurrentIndex(index, QItemSelectionModel::ClearAndSelect);
            return;
        }
    }
    m_statusWidget->setCharacter(nullptr);
    m_inventoryWidget->setCharacter(nullptr);
    m_worldWidget->setCharacter(nullptr);
    m_bonfiresWidget->setCharacter(nullptr);
}

void DS2Widget::onSelectionChange(const QItemSelection& selected, const QItemSelection&) {
    const auto indexes = selected.indexes();
    if (indexes.isEmpty()) {
        m_statusWidget->setCharacter(nullptr);
        m_inventoryWidget->setCharacter(nullptr);
        m_worldWidget->setCharacter(nullptr);
        m_bonfiresWidget->setCharacter(nullptr);
    m_bonfiresWidget->setCharacter(nullptr);
        return;
    }
    showCharacter(indexes.first());
}

void DS2Widget::paintEvent(QPaintEvent* event) {
    QPainter painter = QPainter(this);
    painter.setClipRect(event->rect());
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(6, 5, 7));
    painter.drawRect(rect());
    QPixmap scaled = m_bgPix.scaled(size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    const int x = (width() - scaled.width()) / 2;
    const int y = (height() - scaled.height()) / 2;
    painter.drawPixmap(x, y, scaled);
}
