#include "CopyCharacterDialog.h"

#include <QAbstractItemView>
#include <QComboBox>
#include <QDir>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QGridLayout>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

#include "BaseGameWidget.h"
#include "Controller.h"

namespace fssm::ui {
namespace {
    QString slotText(const fssm::parse::CopyFileInfo& info, const fssm::parse::CopySlotInfo& slot) {
        QString text = QString("Slot %1: ").arg(slot.index + 1);
        if (!slot.occupied) return text + "empty";
        const QString time = QString("%1:%2").arg(slot.playTime / 3600).arg(slot.playTime / 60 % 60, 2, 10, QChar('0'));
        if (info.game == fssm::Game::Sekiro) return text + "Wolf, " + time;
        return text + QString("%1, level %2, %3").arg(QString::fromStdString(slot.name)).arg(slot.level).arg(time);
    }
}

CopyCharacterDialog::CopyCharacterDialog(Mode mode, const QString& path, int slot, QWidget* parent)
    : QDialog(parent), m_mode(mode)
{
    setWindowTitle(mode == Mode::CopyTo ? "Copy character to..." : "Import character from another save...");
    setMinimumWidth(560);

    m_srcFile = new QLabel(this);
    m_dstFile = new QLabel(this);
    m_srcSlot = new QComboBox(this);
    m_dstSlot = new QComboBox(this);
    auto* browseBtn = new QPushButton("Browse...", this);
    m_message = new QLabel(this);
    m_message->setWordWrap(true);

    auto* grid = new QGridLayout();
    grid->addWidget(new QLabel("From:", this), 0, 0);
    grid->addWidget(m_srcFile, 0, 1);
    grid->addWidget(m_srcSlot, 1, 1);
    grid->addWidget(new QLabel("To:", this), 2, 0);
    grid->addWidget(m_dstFile, 2, 1);
    grid->addWidget(m_dstSlot, 3, 1);
    // The side that isn't fixed to the clicked slot can come from any .sl2 file.
    grid->addWidget(browseBtn, mode == Mode::CopyTo ? 2 : 0, 2);
    grid->setColumnStretch(1, 1);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_okBtn = buttons->button(QDialogButtonBox::Ok);
    m_okBtn->setText("Copy...");

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(grid);
    layout->addWidget(m_message);
    layout->addWidget(buttons);

    connect(browseBtn, &QPushButton::clicked, this, &CopyCharacterDialog::onBrowse);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_srcSlot, &QComboBox::currentIndexChanged, this, &CopyCharacterDialog::onSlotsChanged);
    connect(m_dstSlot, &QComboBox::currentIndexChanged, this, &CopyCharacterDialog::onSlotsChanged);

    // Both sides start on the clicked file. Copying fixes the source slot; importing
    // starts on the first empty destination slot (the lists don't show empty slots).
    loadSide(true, path);
    loadSide(false, path);
    if (mode == Mode::CopyTo) {
        m_srcSlot->setCurrentIndex(m_srcSlot->findData(slot));
        m_srcSlot->setEnabled(false);
    }
    onSlotsChanged();
}

void CopyCharacterDialog::loadSide(bool source, const QString& path) {
    QComboBox* combo = source ? m_srcSlot : m_dstSlot;
    QLabel* label = source ? m_srcFile : m_dstFile;
    const int previous = combo->currentData().isValid() ? combo->currentData().toInt() : -1;
    try {
        auto info = fssm::parse::read_copy_file_info(path.toStdString());
        (source ? m_srcInfo : m_dstInfo) = info;
        (source ? m_srcPath : m_dstPath) = path;
    } catch (const std::exception& e) {
        QMessageBox::warning(this, windowTitle(), "Can't use " + path + ":\n" + QString::fromStdString(e.what()));
        return;
    }
    const auto& info = source ? m_srcInfo : m_dstInfo;
    const QString nativePath = QDir::toNativeSeparators(path);
    label->setText(label->fontMetrics().elidedText(nativePath, Qt::ElideMiddle, 460));
    label->setToolTip(nativePath);
    QSignalBlocker blocker(combo);
    combo->clear();
    for (const auto& slot : info.characters) {
        // Only occupied slots can be copied from.
        if (source && !slot.occupied) continue;
        combo->addItem(slotText(info, slot), slot.index);
    }
    int index = combo->findData(previous);
    if (index < 0 && !source) {
        // Default to the first empty slot.
        for (const auto& slot : info.characters) {
            if (!slot.occupied) { index = combo->findData(slot.index); break; }
        }
    }
    combo->setCurrentIndex(index < 0 ? 0 : index);
}

int CopyCharacterDialog::srcSlot() const {
    return m_srcSlot->currentData().isValid() ? m_srcSlot->currentData().toInt() : -1;
}

int CopyCharacterDialog::dstSlot() const {
    return m_dstSlot->currentData().isValid() ? m_dstSlot->currentData().toInt() : -1;
}

bool CopyCharacterDialog::dstOccupied() const {
    const int slot = dstSlot();
    return slot >= 0 && m_dstInfo.characters.at(slot).occupied;
}

QString CopyCharacterDialog::describe(bool source) const {
    const auto& info = source ? m_srcInfo : m_dstInfo;
    const int slot = source ? srcSlot() : dstSlot();
    const QString file = QFileInfo(source ? m_srcPath : m_dstPath).fileName();
    return slotText(info, info.characters.at(slot)) + " of " + file;
}

void CopyCharacterDialog::onBrowse() {
    const bool source = m_mode == Mode::ImportInto;
    const QString current = source ? m_srcPath : m_dstPath;
    const QString path = QFileDialog::getOpenFileName(
        this, source ? "Save file to copy a character from" : "Save file to copy the character into",
        QFileInfo(current).absolutePath(), "Save files (*.sl2);;All files (*)"
    );
    if (path.isEmpty()) return;
    loadSide(source, path);
    onSlotsChanged();
}

void CopyCharacterDialog::onSlotsChanged() {
    QString problem;
    if (m_srcInfo.game != m_dstInfo.game) {
        problem = QString("The files are saves of different games (%1, %2).")
            .arg(m_srcInfo.game.toString(), m_dstInfo.game.toString());
    } else if (srcSlot() < 0) {
        problem = "The source file has no characters.";
    } else if (dstSlot() < 0) {
        problem = "Pick a destination slot.";
    } else if (QFileInfo(m_srcPath) == QFileInfo(m_dstPath) && srcSlot() == dstSlot()) {
        problem = "Source and destination are the same slot.";
    }
    QString text = problem;
    if (problem.isEmpty()) {
        if (m_srcInfo.steamId != m_dstInfo.steamId) {
            text = "The source save belongs to another Steam account; the character's Steam ID is rewritten.";
        }
        if (dstOccupied()) {
            text += (text.isEmpty() ? "" : "\n") + QString("The character in the destination slot will be replaced.");
        }
    }
    m_message->setText(text);
    m_okBtn->setEnabled(problem.isEmpty());
}

void installCharacterCopyMenu(QAbstractItemView* view, int slotRole, Controller* controller,
                              const QString& saveId, BaseGameWidget* gameWidget) {
    view->setContextMenuPolicy(Qt::CustomContextMenu);
    QObject::connect(view, &QWidget::customContextMenuRequested, view, [=](const QPoint& pos) {
        const QModelIndex index = view->indexAt(pos);
        const int slot = index.isValid() ? index.data(slotRole).toInt() : -1;
        QString path;
        for (const auto& item : controller->getSaveFileItems()) {
            if (item.saveId == saveId) path = item.savePath;
        }
        if (path.isEmpty()) return;
        bool occupied = false;
        try {
            const auto info = fssm::parse::read_copy_file_info(path.toStdString());
            occupied = slot >= 0 && info.characters.at(slot).occupied;
        } catch (const std::exception&) {
            return;
        }

        QMenu menu(view);
        QAction* copyTo = occupied ? menu.addAction("Copy character to...") : nullptr;
        menu.addAction("Import character from another save...");
        QAction* chosen = menu.exec(view->viewport()->mapToGlobal(pos));
        if (chosen == nullptr) return;

        using Mode = CopyCharacterDialog::Mode;
        CopyCharacterDialog dialog(chosen == copyTo ? Mode::CopyTo : Mode::ImportInto, path, slot, view->window());
        if (dialog.exec() != QDialog::Accepted) return;

        QString question = "Copy " + dialog.describe(true) + "\ninto " + dialog.describe(false) + "?\n\n";
        if (dialog.dstOccupied()) question += "The character in the destination slot will be lost.\n";
        question += "Close the game first, it would overwrite the change.\n"
                    "A backup of the destination save is made before writing.";
        if (QMessageBox::question(view->window(), dialog.windowTitle(), question,
                                  QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes) {
            return;
        }
        const QString label = QString("Before copying a character into slot %1").arg(dialog.dstSlot() + 1);
        QString backupInfo;
        const QString error = controller->copyCharacter(
            dialog.srcPath(), dialog.srcSlot(), dialog.dstPath(), dialog.dstSlot(), label, backupInfo
        );
        if (!error.isEmpty()) {
            QMessageBox::critical(view->window(), dialog.windowTitle(),
                                  error + (backupInfo.isEmpty() ? "" : "\n\n" + backupInfo));
        } else {
            QMessageBox::information(view->window(), dialog.windowTitle(), "Character copied.\n\n" + backupInfo);
        }
        gameWidget->refresh();
    });
}
}
