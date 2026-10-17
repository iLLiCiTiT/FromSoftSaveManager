#pragma once

#include <QHash>
#include <QPixmap>
#include <QWidget>
#include <optional>

#include "../../parse/DS3/SaveFile.h"

namespace fssm::ui::ds3 {
// "Player Status" panel drawn like the one next to the game's inventory:
// level and attributes on the left, HP/FP/stamina and resistances on the right.
class CharacterInfoWidget: public QWidget {
    Q_OBJECT
public:
    explicit CharacterInfoWidget(QWidget* parent);
    void setCharacter(const fssm::parse::ds3::DS3CharacterInfo* charInfo);
protected:
    void paintEvent(QPaintEvent* event) override;
private:
    QPixmap icon(const QString& name, int size);

    std::optional<fssm::parse::ds3::DS3CharacterInfo> m_character;
    QHash<QString, QPixmap> m_iconCache;
};
}
