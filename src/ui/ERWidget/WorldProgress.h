#pragma once

#include <QWidget>
#include <array>
#include <vector>

#include "../../parse/EldenRing/SaveFile.h"
#include "../ScrollHelper.h"

namespace fssm::ui::er {
// Sites of Grace discovered, bosses defeated (base game, grouped by region) and NPC
// states, from the character's event flags.
class WorldProgressWidget: public QWidget {
    Q_OBJECT
public:
    explicit WorldProgressWidget(QWidget* parent = nullptr);
    void setCharacter(const fssm::parse::er::ERCharacterInfo* charInfo);
protected:
    void paintEvent(QPaintEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;
private:
    bool m_hasCharacter = false;
    int m_index = -1;
    std::vector<bool> m_gracesDiscovered;
    std::vector<bool> m_bossesDefeated;
    std::vector<fssm::parse::er::NpcState> m_npcStates;
    std::vector<std::string_view> m_npcStages;
    // Graces and bosses scroll separately; geometry from the last paint. The lists
    // are not clickable, so dragging them scrolls.
    std::array<fssm::ui::PaintedScroll, 2> m_scrolls {fssm::ui::PaintedScroll(this), fssm::ui::PaintedScroll(this)};
};
}
