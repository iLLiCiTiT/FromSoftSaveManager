#include "DSRWidget.h"

#include <iostream>
#include <qevent.h>
#include <QPainter>
#include <QScrollBar>

#include "Painting.h"
#include "../ManageBackupsWidget.h"

namespace fssm::ui::dsr {
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
    // TODO capture error and use it in first item (NoFlags)
    QStandardItem* root = invisibleRootItem();

    DSRCharInfoResult charsInfo = m_controller->getDsrCharacters(m_saveId);
    if (!charsInfo.error.isEmpty()) {
        m_chars = std::vector<fssm::parse::dsr::DSRCharacterInfo>();
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
        const fssm::parse::dsr::DSRCharacterInfo* character = getCharByIdx(i);
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
        item->setData(QString::fromUtf8(character->areaName.data(), int(character->areaName.size())), CharAreaRole);
    }
    emit refreshed();
};

fssm::parse::dsr::DSRCharacterInfo* CharsListModel::getCharByIdx(const int& index) {
    for (auto& character: m_chars) {
        if (character.index == index) return &character;
    }
    return nullptr;
}
}

namespace {
    // The game's leather menu texture is much darker than the menus look in game (mean
    // grey 6 against 28); lift it to match.
    QPixmap menuBackground() {
        QImage image = QImage(":/dsr_images/bg").convertToFormat(QImage::Format_ARGB32);
        for (int y = 0; y < image.height(); ++y) {
            auto* line = reinterpret_cast<QRgb*>(image.scanLine(y));
            for (int x = 0; x < image.width(); ++x) {
                const QRgb c = line[x];
                const auto lift = [](int v) { return std::min(255, 14 + v * 2); };
                line[x] = qRgba(lift(qRed(c)), lift(qGreen(c)), lift(qBlue(c)), qAlpha(c));
            }
        }
        return QPixmap::fromImage(image);
    }

    // Header icon in the style of the game's framed menu icons: a dark square with a
    // bronze border around `draw`'s picture.
    template <typename Draw>
    QPixmap framedIcon(Draw draw) {
        constexpr int SIZE = 84;
        QPixmap pix(SIZE, SIZE);
        pix.fill(Qt::transparent);
        QPainter painter(&pix);
        painter.setRenderHint(QPainter::Antialiasing, true);
        fssm::ui::dsr::fillLeather(painter, QRect(4, 4, SIZE - 8, SIZE - 8), QColor(30, 27, 24));
        painter.setPen(QPen(fssm::ui::dsr::GOLD_COLOR, 4));
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(QRectF(3, 3, SIZE - 6, SIZE - 6));
        painter.setPen(QPen(fssm::ui::dsr::GOLD_DARK_COLOR, 1.5));
        painter.drawRect(QRectF(7, 7, SIZE - 14, SIZE - 14));
        draw(painter, QRectF(14, 12, SIZE - 28, SIZE - 26));
        return pix;
    }

    QPixmap bonfireIcon() {
        return framedIcon([](QPainter& painter, const QRectF& r) {
            fssm::ui::dsr::drawBonfire(painter, r, true);
        });
    }

    // Gold diamond, the boss mark of the World Progress lists.
    QPixmap worldProgressIcon() {
        return framedIcon([](QPainter& painter, const QRectF& r) {
            const QPointF c = r.center();
            const double h = r.height() * 0.42;
            painter.setPen(QPen(fssm::ui::dsr::GOLD_LIGHT_COLOR, 2.5));
            painter.setBrush(QColor(206, 160, 70));
            painter.drawPolygon(QPolygonF({{c.x(), c.y() - h}, {c.x() + h, c.y()}, {c.x(), c.y() + h}, {c.x() - h, c.y()}}));
        });
    }

    // The game shows the covenant on the Status screen; the faith icon stands in for it.
    QPixmap covenantsIcon() {
        return framedIcon([](QPainter& painter, const QRectF& r) {
            painter.drawPixmap(r.adjusted(-4, -2, 4, 6).toRect(), QPixmap(":/dsr_images/attr_faith"));
        });
    }
}

DSRWidget::DSRWidget(Controller* controller, const QString& saveId, QWidget* parent)
    : BaseGameWidget(controller, saveId, parent)
{
    m_bgPix = menuBackground();

    QWidget* viewWrap = new QWidget(this);
    viewWrap->setAttribute(Qt::WA_TranslucentBackground, true);

    m_view = new QListView(viewWrap);
    m_view->setObjectName("ds_list_view");
    m_view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_view->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_view->setMouseTracking(true);
    m_view->setFrameShape(QFrame::NoFrame);
    m_view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    // Styled in app.qss; the bar is already polished by now, so polish it again with its name.
    QScrollBar* scrollBar = m_view->verticalScrollBar();
    scrollBar->setObjectName("ds_list_scrollbar");
    scrollBar->style()->unpolish(scrollBar);
    scrollBar->style()->polish(scrollBar);
    m_view->setItemDelegate(new fssm::ui::dsr::CharacterDelegate(m_view));
    auto* dataListFrame = new fssm::ui::dsr::DataListFrame(m_view, viewWrap);

    ManageBackupsButtonsWidget* manageBackupsBtnsWidget = new ManageBackupsButtonsWidget(controller, viewWrap);
    manageBackupsBtnsWidget->setAttribute(Qt::WA_TranslucentBackground, true);

    m_model = new fssm::ui::dsr::CharsListModel(controller, saveId, m_view);
    m_view->setModel(m_model);

    QVBoxLayout* viewWrapLayout = new QVBoxLayout(viewWrap);
    viewWrapLayout->setContentsMargins(0, 0, 0, 0);
    viewWrapLayout->addWidget(dataListFrame, 1);
    viewWrapLayout->addWidget(manageBackupsBtnsWidget, 0);

    m_charTabs = new fssm::ui::dsr::HeaderTabs(this);
    m_covenantsWidget = new fssm::ui::dsr::CovenantsWidget(m_charTabs);
    m_charInfoWidget = new fssm::ui::dsr::CharacterInfoWidget(m_charTabs);
    m_inventoryWidget = new fssm::ui::dsr::InventoryWidget(m_charTabs);
    m_worldWidget = new fssm::ui::dsr::WorldProgressWidget(fssm::ui::dsr::WorldPage::Progress, m_charTabs);
    m_bonfiresWidget = new fssm::ui::dsr::WorldProgressWidget(fssm::ui::dsr::WorldPage::Bonfires, m_charTabs);
    m_charTabs->addTab("Character", "Check status", QPixmap(":/dsr_images/menu_status"), m_charInfoWidget);
    m_charTabs->addTab("Inventory", "Select an item to examine", QPixmap(":/dsr_images/menu_inventory"), m_inventoryWidget);
    m_charTabs->addTab("Covenants", "Covenant ranks", covenantsIcon(), m_covenantsWidget);
    m_charTabs->addTab("Progress", "Bosses and NPCs", worldProgressIcon(), m_worldWidget);
    m_charTabs->addTab("Bonfires", "Bonfires lit", bonfireIcon(), m_bonfiresWidget);

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->addWidget(viewWrap, 0);
    layout->addWidget(m_charTabs, 1);

    connect(m_view->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this, SLOT(onSelectionChange(const QItemSelection&, const QItemSelection&)));
    connect(m_model, SIGNAL(refreshed()), this, SLOT(onRefresh()));
    connect(manageBackupsBtnsWidget, SIGNAL(showBackupsRequested()), this, SIGNAL(showBackupsRequested()));
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
    QPixmap scaled = m_bgPix.scaled(size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    // center vertically
    const int y = (height() - scaled.height()) / 2;
    painter.drawPixmap(0, y, scaled);
};

void DSRWidget::onRefresh() {
    // Like the game's list, only slots with a character are shown (or the error in row 0).
    for (int row = 0; row < m_model->rowCount(); ++row) {
        const QModelIndex index = m_model->index(row, 0);
        const bool hasCharacter = index.data(fssm::ui::dsr::CharNameRole).isValid();
        const bool isError = row == 0 && !hasCharacter && !index.data(Qt::DisplayRole).toString().isEmpty();
        m_view->setRowHidden(row, !hasCharacter && !isError);
    }

    QItemSelectionModel* selModel = m_view->selectionModel();
    for (auto& index: selModel->selectedIndexes()) {
        QVariant charId = index.data(fssm::ui::dsr::CharIdRole);
        if (!charId.isValid() || charId.isNull()) continue;
        fssm::parse::dsr::DSRCharacterInfo* charInfo = m_model->getCharByIdx(charId.toInt());
        m_charInfoWidget->setCharacter(charInfo);
        m_inventoryWidget->setCharacter(charInfo);
        m_covenantsWidget->setCharacter(charInfo);
        m_worldWidget->setCharacter(charInfo);
        m_bonfiresWidget->setCharacter(charInfo);
        return;
    }
    for (int row = 0; row < m_model->rowCount(); ++row) {
        QModelIndex index = m_model->index(row, 0);
        if (index.data(fssm::ui::dsr::CharNameRole).isValid()) {
            selModel->setCurrentIndex(index, QItemSelectionModel::ClearAndSelect);
            return;
        }
    }
    selModel->clear();
};

void DSRWidget::onSelectionChange(const QItemSelection &selected, const QItemSelection &deselected) {
    for (auto& index: selected.indexes()) {
        QVariant charId = index.data(fssm::ui::dsr::CharIdRole);
        if (!charId.isValid() || charId.isNull()) continue;
        const fssm::parse::dsr::DSRCharacterInfo* charInfo = m_model->getCharByIdx(charId.toInt());

        m_charInfoWidget->setCharacter(charInfo);
        m_inventoryWidget->setCharacter(charInfo);
        m_covenantsWidget->setCharacter(charInfo);
        m_worldWidget->setCharacter(charInfo);
        m_bonfiresWidget->setCharacter(charInfo);
        return;
    }
    m_charInfoWidget->setCharacter(nullptr);
    m_inventoryWidget->setCharacter(nullptr);
    m_covenantsWidget->setCharacter(nullptr);
    m_worldWidget->setCharacter(nullptr);
    m_bonfiresWidget->setCharacter(nullptr);
}
