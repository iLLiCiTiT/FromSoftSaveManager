#pragma once

#include <QDialog>

#include "../parse/CharacterCopy.h"

class QAbstractItemView;
class QComboBox;
class QLabel;
class QPushButton;
class BaseGameWidget;
class Controller;

namespace fssm::ui {
// Picks the source and destination of a character copy. CopyTo fixes the source to the
// clicked slot and lets the destination file and slot change; ImportInto fixes the
// destination file and lets the source file, source slot and destination slot change.
class CopyCharacterDialog: public QDialog {
    Q_OBJECT
public:
    enum class Mode { CopyTo, ImportInto };
    CopyCharacterDialog(Mode mode, const QString& path, int slot, QWidget* parent);

    QString srcPath() const { return m_srcPath; }
    int srcSlot() const;
    QString dstPath() const { return m_dstPath; }
    int dstSlot() const;
    QString describe(bool source) const;     // "Name (level N) in slot S of file"
    bool dstOccupied() const;

private slots:
    void onBrowse();
    void onSlotsChanged();

private:
    void loadSide(bool source, const QString& path);

    Mode m_mode;
    QString m_srcPath;
    QString m_dstPath;
    fssm::parse::CopyFileInfo m_srcInfo;
    fssm::parse::CopyFileInfo m_dstInfo;
    QLabel* m_srcFile;
    QLabel* m_dstFile;
    QComboBox* m_srcSlot;
    QComboBox* m_dstSlot;
    QLabel* m_message;
    QPushButton* m_okBtn;
};

// Adds "Copy character to..." (on a character) and "Import character from another save..."
// to the context menu of a game's character list. Rows of `view` are the 10 save slots,
// the slot index in `slotRole`.
void installCharacterCopyMenu(QAbstractItemView* view, int slotRole, Controller* controller,
                              const QString& saveId, BaseGameWidget* gameWidget);
}
