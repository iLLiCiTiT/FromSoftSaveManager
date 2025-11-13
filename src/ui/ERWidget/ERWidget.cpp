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
        QStandardItem* item = new QStandardItem("< Empty >");
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        item->setData(QVariant(i), CharIdRole);
        root->appendRow(item);
    }
}
void CharsListModel::refresh() {
    QStandardItem* root = invisibleRootItem();

    ERCharInfoResult charsInfo = m_controller->getERCharacters(m_saveId);
    if (!charsInfo.error.isEmpty()) {
        m_chars = std::vector<fssm::parse::er::ERCharacterInfo>();
        QStandardItem* item = root->child(0);
        item->setText(charsInfo.error);
        for (int i = 1; i < root->rowCount(); ++i) {
            QStandardItem* item = root->child(i);
            item->setText("");
        }
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
                found = true;
                break;
            }
        }
        if (found) continue;
        item->setText("< Empty >");
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
    m_view->setObjectName("ds_list_view");
    m_view->setAttribute(Qt::WA_TranslucentBackground, true);
    m_view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_view->setTextElideMode(Qt::ElideLeft);
    m_view->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

    m_model = new fssm::ui::er::CharsListModel(controller, saveId, m_view);
    m_view->setModel(m_model);

    ManageBackupsButtonsWidget* manageBackupsBtnsWidget = new ManageBackupsButtonsWidget(controller, viewWrap);

    QVBoxLayout* viewWrapLayout = new QVBoxLayout(viewWrap);
    viewWrapLayout->setContentsMargins(0, 0, 0, 0);
    viewWrapLayout->addSpacing(28);
    viewWrapLayout->addWidget(m_view, 4);
    viewWrapLayout->addWidget(manageBackupsBtnsWidget, 1);

    m_charTabs = new TabWidget(this);

    m_charInfoWidget = new fssm::ui::er::CharacterInfoWidget(m_charTabs);

    m_charTabs->addTab(
        "Character Info",
        m_charInfoWidget
    );

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
    QItemSelectionModel* selModel = m_view->selectionModel();
    for (auto& index: selModel->selectedIndexes()) {
        QVariant charId = index.data(fssm::ui::er::CharIdRole);
        if (!charId.isValid() || charId.isNull()) continue;
        const fssm::parse::er::ERCharacterInfo* charInfo = m_model->getCharByIdx(charId.toInt());
        m_charInfoWidget->setCharacter(charInfo);
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
}

void ERWidget::onSelectionChange(const QItemSelection& selected, const QItemSelection& deselected) {
    for (auto& index: selected.indexes()) {
        QVariant charId = index.data(fssm::ui::er::CharIdRole);
        if (!charId.isValid() || charId.isNull()) continue;
        const fssm::parse::er::ERCharacterInfo* charInfo = m_model->getCharByIdx(charId.toInt());

        m_charInfoWidget->setCharacter(charInfo);
        return;
    }
    m_charInfoWidget->setCharacter(nullptr);

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