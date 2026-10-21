#include "ERWidget.h"

#include <QPainter>
#include <qevent.h>

#include "../BaseGameWidget.h"
#include "../ManageBackupsWidget.h"

namespace fssm::ui::er {
CharsListModel::CharsListModel(Controller* controller, const QString& saveId, QObject* parent)
    : QStandardItemModel(parent),
    m_controller(controller),
    m_saveId(saveId)

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

    ERCharInfoResult charsInfo = m_controller->getERCharacters(m_saveId);
    if (!charsInfo.error.isEmpty()) {
        m_chars = std::vector<fssm::parse::er::ERCharacterInfo>();
        for (int i = 0; i < root->rowCount(); ++i) {
            QStandardItem* item = root->child(i);
            item->setText(i == 0 ? charsInfo.error : "");
            item->setData(QVariant(), CharNameRole);
        }
        emit refreshed();
        return;
    }
    m_chars = charsInfo.characters;
    for (int i = 0; i < root->rowCount(); ++i) {
        QStandardItem* item = root->child(i);
        const fssm::parse::er::ERCharacterInfo* character = getCharByIdx(i);
        if (character == nullptr) {
            item->setText("");
            item->setData(QVariant(), CharNameRole);
            continue;
        }
        const QString name = QString::fromStdU16String(character->name);
        item->setText(name);
        item->setData(name, CharNameRole);
        item->setData(character->level, CharLevelRole);
        item->setData(character->playTime, CharPlayTimeRole);
        item->setData(QString::fromUtf8(character->placeName.data(), qsizetype(character->placeName.size())),
                      CharAreaRole);
    }
    emit refreshed();
}

const fssm::parse::er::ERCharacterInfo* CharsListModel::getCharByIdx(const int& index) const {
    for (auto& character: m_chars) {
        if (character.index == index) return &character;
    }
    return nullptr;
}
};

ERWidget::ERWidget(Controller* controller, const QString& saveId, QWidget* parent)
    : BaseGameWidget(controller, saveId, parent)
{
    QWidget* viewWrap = new QWidget(this);
    viewWrap->setAttribute(Qt::WA_TranslucentBackground, true);

    m_view = new QListView(viewWrap);
    m_view->setObjectName("er_list_view");
    m_view->setAttribute(Qt::WA_TranslucentBackground, true);
    m_view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_view->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_view->setMouseTracking(true);
    m_view->setFrameShape(QFrame::NoFrame);
    m_view->setItemDelegate(new fssm::ui::er::CharacterDelegate(m_view));
    auto* loadGameFrame = new fssm::ui::er::LoadGameFrame(m_view, viewWrap);

    m_model = new fssm::ui::er::CharsListModel(controller, saveId, m_view);
    m_view->setModel(m_model);

    ManageBackupsButtonsWidget* manageBackupsBtnsWidget = new ManageBackupsButtonsWidget(controller, viewWrap);
    manageBackupsBtnsWidget->setAttribute(Qt::WA_TranslucentBackground, true);

    QVBoxLayout* viewWrapLayout = new QVBoxLayout(viewWrap);
    viewWrapLayout->setContentsMargins(0, 0, 0, 0);
    viewWrapLayout->addWidget(loadGameFrame, 1);
    viewWrapLayout->addWidget(manageBackupsBtnsWidget, 0);

    m_charTabs = new TabWidget(this);

    m_charInfoWidget = new fssm::ui::er::CharacterInfoWidget(m_charTabs);

    m_charTabs->addTab(
        "Character Info",
        m_charInfoWidget
    );
    m_inventoryWidget = new fssm::ui::er::InventoryWidget(m_charTabs);
    m_charTabs->addTab("Inventory", m_inventoryWidget);
    m_worldWidget = new fssm::ui::er::WorldProgressWidget(m_charTabs);
    m_charTabs->addTab("World Progress", m_worldWidget);

    QHBoxLayout* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->addWidget(viewWrap, 0);
    mainLayout->addWidget(m_charTabs, 1);

    connect(m_view->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this, SLOT(onSelectionChange(const QItemSelection&, const QItemSelection&)));
    connect(m_model, SIGNAL(refreshed()), this, SLOT(onRefresh()));
    connect(manageBackupsBtnsWidget, SIGNAL(showBackupsRequested()), this, SIGNAL(showBackupsRequested()));
}

void ERWidget::refresh() {
    m_model->refresh();
}

void ERWidget::onRefresh() {
    // Like the game's list, only slots with a character are shown (or the error in row 0).
    for (int row = 0; row < m_model->rowCount(); ++row) {
        const QModelIndex index = m_model->index(row, 0);
        const bool hasCharacter = index.data(fssm::ui::er::CharNameRole).isValid();
        const bool isError = row == 0 && !hasCharacter && !index.data(Qt::DisplayRole).toString().isEmpty();
        m_view->setRowHidden(row, !hasCharacter && !isError);
    }

    QItemSelectionModel* selModel = m_view->selectionModel();
    for (auto& index: selModel->selectedIndexes()) {
        QVariant charId = index.data(fssm::ui::er::CharIdRole);
        if (!charId.isValid() || charId.isNull()) continue;
        const fssm::parse::er::ERCharacterInfo* charInfo = m_model->getCharByIdx(charId.toInt());
        m_charInfoWidget->setCharacter(charInfo);
        m_inventoryWidget->setCharacter(charInfo);
        m_worldWidget->setCharacter(charInfo);
        return;
    }
    for (int row = 0; row < m_model->rowCount(); ++row) {
        QModelIndex index = m_model->index(row, 0);
        if (index.data(fssm::ui::er::CharNameRole).isValid()) {
            selModel->setCurrentIndex(index, QItemSelectionModel::ClearAndSelect);
            return;
        }
    }
    selModel->clear();
}

void ERWidget::onSelectionChange(const QItemSelection& selected, const QItemSelection& deselected) {
    for (auto& index: selected.indexes()) {
        QVariant charId = index.data(fssm::ui::er::CharIdRole);
        if (!charId.isValid() || charId.isNull()) continue;
        const fssm::parse::er::ERCharacterInfo* charInfo = m_model->getCharByIdx(charId.toInt());

        m_charInfoWidget->setCharacter(charInfo);
        m_inventoryWidget->setCharacter(charInfo);
        m_worldWidget->setCharacter(charInfo);
        return;
    }
    m_charInfoWidget->setCharacter(nullptr);
    m_inventoryWidget->setCharacter(nullptr);
    m_worldWidget->setCharacter(nullptr);

}

void ERWidget::paintEvent(QPaintEvent* event) {
    QPainter painter = QPainter(this);
    painter.setClipRect(event->rect());
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0, 0, 0));
    painter.drawRect(rect());
    QPixmap pix = QPixmap(":/er_images/bg");
    QSize currentSize = size() * 0.7;
    QPixmap scaled = pix.scaled(currentSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    const int x = (width() - scaled.width()) / 2;
    const int y = (height() - scaled.height()) / 2;
    painter.drawPixmap(x, y, scaled);

    QPixmap pixOverlay = QPixmap(":/er_images/bg_overlay");
    painter.drawPixmap(0, 0, pixOverlay.scaled(size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
}