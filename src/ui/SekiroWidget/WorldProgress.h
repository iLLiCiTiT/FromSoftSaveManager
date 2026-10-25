#pragma once

#include <QWidget>
#include <array>
#include <vector>

#include "Painting.h"
#include "../ScrollHelper.h"
#include "../../parse/Sekiro/SaveFile.h"

namespace fssm::ui::sekiro {
// What a WorldProgressWidget shows.
enum class WorldPage {
    // Bosses defeated (grouped by the travel menu's areas) with the prosthetic tools found,
    // and NPC progress (state, questline stage and steps), in two detail panels.
    Progress,
    Idols,      // Sculptor's Idols found, grouped by the travel menu's areas
};

// One page of a character's world progress, from its event flags and inventory.
// Shown as a page of the inventory screen.
class WorldProgressWidget: public QWidget {
    Q_OBJECT
public:
    explicit WorldProgressWidget(WorldPage page, QWidget* parent = nullptr);
    void setProgress(const fssm::parse::sekiro::SekiroCharacterInfo& character);
protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;
private:
    WorldPage m_page;
    std::vector<bool> m_idolsFound;
    std::vector<bool> m_bossesDefeated;
    std::vector<bool> m_toolsFound;
    std::vector<fssm::parse::sekiro::NpcState> m_npcStates;
    std::vector<std::string_view> m_npcStages;
    std::vector<bool> m_npcDragonrot;
    std::vector<bool> m_npcStepsDone;
    // The two panels scroll separately (the idols page uses the first);
    // geometry from the last paint. The lists are not clickable, so dragging them scrolls.
    std::array<fssm::ui::PaintedScroll, 2> m_scrolls {fssm::ui::PaintedScroll(this), fssm::ui::PaintedScroll(this)};
    PixmapCache m_cache;
};
}
