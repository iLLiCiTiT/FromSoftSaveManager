#pragma once

#include <QWidget>
#include <optional>

#include "../../parse/DSR/SaveFile.h"

namespace fssm::ui::dsr {
// Lit bonfires (grouped by area) and defeated bosses of a character.
class WorldProgressWidget: public QWidget {
    Q_OBJECT
public:
    explicit WorldProgressWidget(QWidget* parent);
    void setCharacter(const fssm::parse::dsr::DSRCharacterInfo* charInfo);
protected:
    void paintEvent(QPaintEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
private:
    std::optional<fssm::parse::dsr::DSRCharacterInfo> m_character;
    int m_scroll = 0;
    // From the last paint: scrollable range and one scroll step.
    int m_maxScroll = 0;
    int m_lineHeight = 22;
};
}
