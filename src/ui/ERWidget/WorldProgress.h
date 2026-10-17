#pragma once

#include <QWidget>
#include <array>
#include <vector>

#include "../../parse/EldenRing/SaveFile.h"

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
private:
    bool m_hasCharacter = false;
    int m_index = -1;
    std::vector<bool> m_gracesDiscovered;
    std::vector<bool> m_bossesDefeated;
    std::vector<fssm::parse::er::NpcState> m_npcStates;
    std::vector<std::string_view> m_npcStages;
    // Graces and bosses scroll separately. From the last paint: each list's
    // rectangle, scrollable range and one scroll step.
    std::array<int, 2> m_scroll {};
    std::array<int, 2> m_maxScroll {};
    std::array<QRect, 2> m_lists;
    int m_lineHeight = 24;
};
}
