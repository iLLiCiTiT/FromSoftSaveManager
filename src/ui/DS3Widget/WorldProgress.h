#pragma once

#include <QWidget>
#include <optional>

#include "../ScrollHelper.h"
#include "../../parse/DS3/SaveFile.h"

namespace fssm::ui::ds3 {
// Lit bonfires (grouped by the travel menu's areas), defeated bosses and NPC states of a
// character, from its event flags.
class WorldProgressWidget: public QWidget {
    Q_OBJECT
public:
    explicit WorldProgressWidget(QWidget* parent);
    void setCharacter(const fssm::parse::ds3::DS3CharacterInfo* charInfo);
protected:
    void paintEvent(QPaintEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;
private:
    std::optional<fssm::parse::ds3::DS3CharacterInfo> m_character;
    // Geometry from the last paint; the lists are not clickable, so dragging them scrolls.
    fssm::ui::PaintedScroll m_scroll {this};
};
}
