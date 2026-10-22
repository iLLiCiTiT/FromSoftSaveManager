#pragma once

#include <QWidget>
#include <array>
#include <vector>

#include "Painting.h"
#include "../../parse/Sekiro/SaveFile.h"

namespace fssm::ui::sekiro {
// Sculptor's Idols found (grouped by the travel menu's areas), and in the
// detail panel the bosses defeated, prosthetic tools found and NPC progress
// (state, questline stage and steps), from the character's event flags and
// inventory. Shown as a page of the inventory screen.
class WorldProgressWidget: public QWidget {
    Q_OBJECT
public:
    explicit WorldProgressWidget(QWidget* parent = nullptr);
    void setProgress(const fssm::parse::sekiro::SekiroCharacterInfo& character);
protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
private:
    std::vector<bool> m_idolsFound;
    std::vector<bool> m_bossesDefeated;
    std::vector<bool> m_toolsFound;
    std::vector<fssm::parse::sekiro::NpcState> m_npcStates;
    std::vector<std::string_view> m_npcStages;
    std::vector<bool> m_npcStepsDone;
    // Idols and the detail panel scroll separately. From the last paint: each list's
    // rectangle, scrollable range and one scroll step.
    std::array<int, 2> m_scroll {};
    std::array<int, 2> m_maxScroll {};
    std::array<QRect, 2> m_lists;
    int m_lineHeight = 26;
    PixmapCache m_cache;
};
}
