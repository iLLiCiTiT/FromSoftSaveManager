#pragma once

#include <QPixmap>
#include <QWidget>
#include <array>

#include "../../parse/DS2/SaveFile.h"
#include "../ScrollHelper.h"

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
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    bool m_hasCharacter = false;
    fssm::parse::ds2::DS2CharacterInfo m_char;
    QPixmap m_window;
    // Bonfires and bosses scroll separately; geometry from the last paint. The lists
    // are not clickable, so dragging them scrolls.
    std::array<fssm::ui::PaintedScroll, 2> m_scrolls {fssm::ui::PaintedScroll(this), fssm::ui::PaintedScroll(this)};
};
}
