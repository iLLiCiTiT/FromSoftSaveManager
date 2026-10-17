#pragma once

#include <QPixmap>
#include <QWidget>
#include <array>

#include "../../parse/DS2/SaveFile.h"

namespace fssm::ui::ds2 {
// Lit bonfires and defeated bosses of a character, grouped by area, in the
// game's window frame like the Player Status screen.
class WorldProgressWidget: public QWidget {
    Q_OBJECT
public:
    explicit WorldProgressWidget(QWidget* parent = nullptr);
    void setCharacter(const fssm::parse::ds2::DS2CharacterInfo* charInfo);
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    bool m_hasCharacter = false;
    fssm::parse::ds2::DS2CharacterInfo m_char;
    QPixmap m_window;
    // Bonfires and bosses scroll separately. From the last paint: each list's
    // rectangle, scrollable range and one scroll step.
    std::array<int, 2> m_scroll {};
    std::array<int, 2> m_maxScroll {};
    std::array<QRect, 2> m_lists;
    int m_lineHeight = 24;
};
}
