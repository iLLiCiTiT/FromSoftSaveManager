#include "SekiroWidget.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QVBoxLayout>
#include <qevent.h>

#include "Painting.h"
#include "../ManageBackupsWidget.h"

namespace fssm::ui::sekiro {
namespace {
    // bg.png (the game's MENU_Base_00 at 1920x1080): borders kept unscaled by the
    // nine-slice, the bottom one includes the ink rule above the key guide.
    constexpr int BG_W = 1920;
    constexpr int BG_H = 1080;
    constexpr int FRAME_LEFT = 80;
    constexpr int FRAME_TOP = 64;
    constexpr int FRAME_RIGHT = 80;
    constexpr int FRAME_BOTTOM = 130;

    // Title of the character list, drawn like the game's "LOAD GAME" heading.
    class ListTitle: public QWidget {
    public:
        explicit ListTitle(QWidget* parent): QWidget(parent) {
            setAttribute(Qt::WA_TranslucentBackground, true);
            setFixedHeight(36);
        }
    protected:
        void paintEvent(QPaintEvent*) override {
            QPainter painter(this);
            painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
            const QRect bar = rect().adjusted(0, 2, -width() / 4, -2);
            painter.drawPixmap(bar.topLeft(), m_cache.get(":/sekiro_images/title_bar", bar.size()));
            painter.setFont(serifFont(20));
            drawShadowText(painter, bar.adjusted(22, 0, 0, 0), Qt::AlignLeft | Qt::AlignVCenter, "LOAD GAME");
        }
    private:
        PixmapCache m_cache;
    };
}

CharsListModel::CharsListModel(Controller* controller, const QString& saveId, QObject* parent)
    : QStandardItemModel(parent),
    m_saveId(saveId),
    m_controller(controller)
{
    setColumnCount(1);
    QStandardItem* root = invisibleRootItem();
    for (int i = 0; i < int(fssm::parse::sekiro::CHARACTER_SLOTS); ++i) {
        QStandardItem* item = new QStandardItem("");
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        item->setData(QVariant(), CharAreaRole);
        item->setData(QVariant(i), CharIdRole);
        root->appendRow(item);
    }
}

void CharsListModel::refresh() {
    QStandardItem* root = invisibleRootItem();

    SekiroCharInfoResult charsInfo = m_controller->getSekiroCharacters(m_saveId);
    if (!charsInfo.error.isEmpty()) {
        m_chars.clear();
        for (int i = 0; i < root->rowCount(); ++i) {
            root->child(i)->setText(i == 0 ? charsInfo.error : "");
            root->child(i)->setData(QVariant(), CharAreaRole);
        }
        emit refreshed();
        return;
    }
    m_chars = charsInfo.characters;
    for (int i = 0; i < root->rowCount(); ++i) {
        QStandardItem* item = root->child(i);
        const fssm::parse::sekiro::SekiroCharacterInfo* character = getCharByIdx(i);
        if (character == nullptr) {
            item->setText("");
            item->setData(QVariant(), CharAreaRole);
            continue;
        }
        const auto area = fssm::parse::sekiro::findPlaceName(character->placeNameId);
        item->setText(QString("Profile %1").arg(i + 1));
        item->setData(area.empty() ? QString("-") : QString::fromUtf8(area.data(), int(area.size())), CharAreaRole);
        item->setData(character->playTime, CharPlayTimeRole);
        item->setData(QVariant::fromValue<qlonglong>(character->lastSaved), CharLastSavedRole);
        item->setData(character->vitality, CharVitalityRole);
        item->setData(character->attackPower, CharAttackPowerRole);
    }
    emit refreshed();
}

const fssm::parse::sekiro::SekiroCharacterInfo* CharsListModel::getCharByIdx(const int& index) const {
    for (auto& character: m_chars) {
        if (character.index == index) return &character;
    }
    return nullptr;
}
}

SekiroWidget::SekiroWidget(Controller* controller, const QString& saveId, QWidget* parent)
    : BaseGameWidget(controller, saveId, parent)
{
    setAttribute(Qt::WA_TranslucentBackground, true);
    m_bgPix = QPixmap(":/sekiro_images/bg");

    QWidget* viewWrap = new QWidget(this);
    viewWrap->setAttribute(Qt::WA_TranslucentBackground, true);

    m_view = new QListView(viewWrap);
    m_view->setObjectName("sekiro_list_view");
    m_view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_view->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_view->setMouseTracking(true);
    m_view->setFrameShape(QFrame::NoFrame);
    m_view->setItemDelegate(new fssm::ui::sekiro::CharacterDelegate(m_view));
    m_view->setMinimumWidth(250);
    viewWrap->setFixedWidth(280);

    ManageBackupsButtonsWidget* manageBackupsBtnsWidget = new ManageBackupsButtonsWidget(controller, viewWrap);
    manageBackupsBtnsWidget->setAttribute(Qt::WA_TranslucentBackground, true);

    m_model = new fssm::ui::sekiro::CharsListModel(controller, saveId, m_view);
    m_view->setModel(m_model);

    QVBoxLayout* viewWrapLayout = new QVBoxLayout(viewWrap);
    viewWrapLayout->setContentsMargins(0, 0, 0, 0);
    viewWrapLayout->setSpacing(6);
    viewWrapLayout->addWidget(new fssm::ui::sekiro::ListTitle(viewWrap), 0);
    viewWrapLayout->addWidget(m_view, 4);
    viewWrapLayout->addWidget(manageBackupsBtnsWidget, 1);

    m_inventoryWidget = new fssm::ui::sekiro::InventoryWidget(this);

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setSpacing(16);
    layout->addWidget(viewWrap, 0);
    layout->addWidget(m_inventoryWidget, 1);

    connect(m_view->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
            this, SLOT(onSelectionChange(const QItemSelection&, const QItemSelection&)));
    connect(m_model, SIGNAL(refreshed()), this, SLOT(onRefresh()));
    connect(manageBackupsBtnsWidget, SIGNAL(showBackupsRequested()), this, SIGNAL(showBackupsRequested()));
}

void SekiroWidget::refresh() {
    m_model->refresh();
}

double SekiroWidget::frameScale() const {
    using namespace fssm::ui::sekiro;
    return std::clamp(std::min(double(width()) / BG_W, double(height()) / BG_H), 0.3, 1.0);
}

void SekiroWidget::resizeEvent(QResizeEvent* event) {
    using namespace fssm::ui::sekiro;
    QWidget::resizeEvent(event);
    // Keep the content inside the board of the frame.
    const double s = frameScale();
    layout()->setContentsMargins(int(FRAME_LEFT * s) + 14, int(FRAME_TOP * s) + 10,
                                 int(FRAME_RIGHT * s) + 14, int(FRAME_BOTTOM * s) - 8);
}

void SekiroWidget::showCharacter(const QModelIndex& index) {
    const QVariant charId = index.data(fssm::ui::sekiro::CharIdRole);
    const fssm::parse::sekiro::SekiroCharacterInfo* charInfo = nullptr;
    if (charId.isValid() && !charId.isNull()) charInfo = m_model->getCharByIdx(charId.toInt());
    m_inventoryWidget->setCharacter(charInfo);
}

void SekiroWidget::onRefresh() {
    // Like the game's list, only slots with a character are shown (or the error in row 0).
    for (int row = 0; row < m_model->rowCount(); ++row) {
        const QModelIndex index = m_model->index(row, 0);
        const bool hasCharacter = index.data(fssm::ui::sekiro::CharAreaRole).isValid();
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
        if (index.data(fssm::ui::sekiro::CharAreaRole).isValid()) {
            selModel->setCurrentIndex(index, QItemSelectionModel::ClearAndSelect);
            return;
        }
    }
    m_inventoryWidget->setCharacter(nullptr);
}

void SekiroWidget::onSelectionChange(const QItemSelection& selected, const QItemSelection&) {
    const auto indexes = selected.indexes();
    if (indexes.isEmpty()) {
        m_inventoryWidget->setCharacter(nullptr);
        return;
    }
    showCharacter(indexes.first());
}

void SekiroWidget::paintEvent(QPaintEvent* event) {
    using namespace fssm::ui::sekiro;
    QPainter painter = QPainter(this);
    painter.setClipRect(event->rect());
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(6, 5, 7));
    painter.drawRect(rect());
    const double s = frameScale();
    if (!m_bgPix.isNull()) {
        drawNineSlice(painter, m_bgPix, rect(), FRAME_LEFT, FRAME_TOP, FRAME_RIGHT, FRAME_BOTTOM, s);
    }
    // The game lights the board noticeably brighter than the raw texture.
    const QRect board = rect().adjusted(int(72 * s), int(56 * s), -int(54 * s), -int(56 * s));
    painter.setCompositionMode(QPainter::CompositionMode_Screen);
    painter.fillRect(board, QColor(120, 116, 108));
}
