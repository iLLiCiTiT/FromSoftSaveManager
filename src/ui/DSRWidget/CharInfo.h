#pragma once

#include <QHash>
#include <QPixmap>
#include <QWidget>
#include <optional>

#include "../ScrollHelper.h"
#include "../../parse/DSR/SaveFile.h"

namespace fssm::ui::dsr {
// Covenant name with its rank ("Warrior of Sunlight+2"), as on the Status screen.
QString covenantTitle(const fssm::parse::dsr::DSRCharacterInfo& charInfo);

// The game's Status screen: framed panels with the name, covenant, level and attributes;
// HP, stamina, resistances and attunement slots; and the character's creation details.
// Panels sit side by side when they fit; the page scrolls when it is too short.
class CharacterInfoWidget: public QWidget {
    Q_OBJECT
public:
    explicit CharacterInfoWidget(QWidget* parent);
    void setCharacter(const fssm::parse::dsr::DSRCharacterInfo* charInfo);
protected:
    void paintEvent(QPaintEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;
private:
    QPixmap icon(const QString& resource, int size);

    std::optional<fssm::parse::dsr::DSRCharacterInfo> m_character;
    QHash<QString, QPixmap> m_iconCache;
    fssm::ui::PaintedScroll m_scroll {this};
};
}
