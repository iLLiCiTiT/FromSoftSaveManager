#pragma once

#include <QWidget>
#include <optional>

#include "../ScrollHelper.h"
#include "../../parse/DS3/SaveFile.h"

namespace fssm::ui::ds3 {
// What a WorldProgressWidget shows.
enum class WorldPage {
    Progress,   // defeated bosses and NPC states, side by side, each scrolling on its own
    Bonfires,   // lit bonfires, grouped by the travel menu's areas
};

// One page of a character's world progress, from its event flags.
class WorldProgressWidget: public QWidget {
    Q_OBJECT
public:
    explicit WorldProgressWidget(WorldPage page, QWidget* parent);
    void setCharacter(const fssm::parse::ds3::DS3CharacterInfo* charInfo);
protected:
    void paintEvent(QPaintEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;
private:
    WorldPage m_page;
    std::optional<fssm::parse::ds3::DS3CharacterInfo> m_character;
    // Geometry from the last paint; the lists are not clickable, so dragging them scrolls.
    // The bonfires page and the bosses use m_scroll, the NPCs m_npcScroll.
    fssm::ui::PaintedScroll m_scroll {this};
    fssm::ui::PaintedScroll m_npcScroll {this};
};
}
