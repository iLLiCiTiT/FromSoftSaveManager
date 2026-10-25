#pragma once

#include <QPixmap>
#include <QWidget>
#include <array>

#include "../../parse/DS2/SaveFile.h"
#include "../ScrollHelper.h"

namespace fssm::ui::ds2 {
// What a WorldProgressWidget shows.
enum class WorldPage {
    // Defeated bosses, and an NPCs panel: the save has no NPC data we can read (see
    // the Dark Souls II section of docs/event-flags.md).
    Progress,
    Bonfires,   // lit bonfires
};

// One page of a character's world progress, grouped by area, in the game's window
// frame like the Player Status screen.
class WorldProgressWidget: public QWidget {
    Q_OBJECT
public:
    explicit WorldProgressWidget(WorldPage page, QWidget* parent = nullptr);
    void setCharacter(const fssm::parse::ds2::DS2CharacterInfo* charInfo);
    QSize sizeHint() const override;

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
    fssm::parse::ds2::DS2CharacterInfo m_char;
    QPixmap m_window;
    // Bosses and NPCs scroll separately (the bonfires page uses the first); geometry from
    // the last paint. The lists are not clickable, so dragging them scrolls.
    std::array<fssm::ui::PaintedScroll, 2> m_scrolls {fssm::ui::PaintedScroll(this), fssm::ui::PaintedScroll(this)};
};
}
