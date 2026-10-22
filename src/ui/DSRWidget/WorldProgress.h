#pragma once

#include <QWidget>
#include <optional>

#include "../ScrollHelper.h"
#include "../../parse/DSR/SaveFile.h"

namespace fssm::ui::dsr {
// Lit bonfires (grouped by area), defeated bosses and NPC states of a character.
class WorldProgressWidget: public QWidget {
    Q_OBJECT
public:
    explicit WorldProgressWidget(QWidget* parent);
    void setCharacter(const fssm::parse::dsr::DSRCharacterInfo* charInfo);
protected:
    void paintEvent(QPaintEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;
private:
    std::optional<fssm::parse::dsr::DSRCharacterInfo> m_character;
    // Geometry from the last paint; the lists are not clickable, so dragging them scrolls.
    fssm::ui::PaintedScroll m_scroll {this};
};
}
