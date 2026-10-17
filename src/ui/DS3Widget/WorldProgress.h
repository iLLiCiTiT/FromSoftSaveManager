#pragma once

#include <QWidget>
#include <optional>

#include "../../parse/DS3/SaveFile.h"

namespace fssm::ui::ds3 {
// Lit bonfires (grouped by the travel menu's areas) and defeated bosses of a
// character, from its event flags.
class WorldProgressWidget: public QWidget {
    Q_OBJECT
public:
    explicit WorldProgressWidget(QWidget* parent);
    void setCharacter(const fssm::parse::ds3::DS3CharacterInfo* charInfo);
protected:
    void paintEvent(QPaintEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
private:
    std::optional<fssm::parse::ds3::DS3CharacterInfo> m_character;
    int m_scroll = 0;
    // From the last paint: scrollable range and one scroll step.
    int m_maxScroll = 0;
    int m_lineHeight = 26;
};
}
