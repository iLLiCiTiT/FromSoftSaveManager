#pragma once

#include <QWidget>
#include <array>
#include <vector>

#include "Painting.h"

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
private:
    std::vector<bool> m_idolsFound;
    std::vector<bool> m_bossesDefeated;
    // Idols and bosses scroll separately. From the last paint: each list's
    // rectangle, scrollable range and one scroll step.
    std::array<int, 2> m_scroll {};
    std::array<int, 2> m_maxScroll {};
    std::array<QRect, 2> m_lists;
    int m_lineHeight = 26;
    PixmapCache m_cache;
};
}
