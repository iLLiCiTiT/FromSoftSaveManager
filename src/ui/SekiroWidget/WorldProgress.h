#pragma once

#include <QWidget>
#include <array>
#include <vector>

#include "Painting.h"
#include "../ScrollHelper.h"

namespace fssm::ui::sekiro {
// Sculptor's Idols found (grouped by the travel menu's areas) and bosses
// defeated, from the character's event flags. Shown as a page of the
// inventory screen.
class WorldProgressWidget: public QWidget {
    Q_OBJECT
public:
    explicit WorldProgressWidget(QWidget* parent = nullptr);
    void setProgress(const std::vector<bool>& idolsFound, const std::vector<bool>& bossesDefeated);
protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;
private:
    std::vector<bool> m_idolsFound;
    std::vector<bool> m_bossesDefeated;
    // Idols and bosses scroll separately; geometry from the last paint. The lists
    // are not clickable, so dragging them scrolls.
    std::array<fssm::ui::PaintedScroll, 2> m_scrolls {fssm::ui::PaintedScroll(this), fssm::ui::PaintedScroll(this)};
    PixmapCache m_cache;
};
}
