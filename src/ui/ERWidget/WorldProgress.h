#pragma once

#include <QWidget>
#include <array>
#include <vector>

#include "../../parse/EldenRing/SaveFile.h"
#include "../ScrollHelper.h"

class QPainter;
class QRectF;

namespace fssm::ui::er {
// Site of Grace: a golden glow with a spark when discovered, a faint ring otherwise.
void drawGraceMark(QPainter& painter, const QRectF& rect, bool discovered);

// What a WorldProgressWidget shows.
enum class WorldPage {
    Progress,   // bosses defeated (base game, grouped by region); NPC states with their questline progress and endings
    Graces,     // Sites of Grace discovered, grouped by region
};

// One page of a character's world progress, from its event flags.
class WorldProgressWidget: public QWidget {
    Q_OBJECT
public:
    explicit WorldProgressWidget(WorldPage page, QWidget* parent = nullptr);
    void setCharacter(const fssm::parse::er::ERCharacterInfo* charInfo);
protected:
    void paintEvent(QPaintEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;
private:
    WorldPage m_page;
    bool m_hasCharacter = false;
    int m_index = -1;
    std::vector<bool> m_gracesDiscovered;
    std::vector<bool> m_bossesDefeated;
    std::vector<fssm::parse::er::NpcState> m_npcStates;
    std::vector<std::string_view> m_npcStages;
    std::vector<bool> m_npcStepsDone;
    std::vector<fssm::parse::er::EndingState> m_endings;
    // Bosses and NPCs scroll separately (the graces page uses the first); geometry from
    // the last paint. The lists are not clickable, so dragging them scrolls.
    std::array<fssm::ui::PaintedScroll, 2> m_scrolls {fssm::ui::PaintedScroll(this), fssm::ui::PaintedScroll(this)};
};
}
