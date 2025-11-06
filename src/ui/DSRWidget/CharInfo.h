#pragma once

#include <QLabel>
#include "../../parse/Parse.h"

namespace fssm::ui::dsr {
class CharacterStatusWidget: public QWidget {
    Q_OBJECT
public:
    explicit CharacterStatusWidget(QWidget* parent);
    void setCharacter(const fssm::parse::dsr::DSRCharacterInfo* charInfo);
private:
    void setEmpty();
    QLabel* m_nameValueWidget = nullptr;
    QLabel* m_covenantValueWidget = nullptr;
    QLabel* m_levelValueWidget = nullptr;
    QLabel* m_soulsValueWidget = nullptr;
    QLabel* m_vitalityValueWidget = nullptr;
    QLabel* m_attunementValueWidget = nullptr;
    QLabel* m_enduranceValueWidget = nullptr;
    QLabel* m_strengthValueWidget = nullptr;
    QLabel* m_dexterityValueWidget = nullptr;
    QLabel* m_resistanceValueWidget = nullptr;
    QLabel* m_intelligenceValueWidget = nullptr;
    QLabel* m_faithValueWidget = nullptr;
    QLabel* m_humanityValueWidget = nullptr;

    QLabel* m_hollowValueWidget = nullptr;
    QLabel* m_sexValueWidget = nullptr;
    QLabel* m_classValueWidget = nullptr;
    QLabel* m_physiqueValueWidget = nullptr;
    QLabel* m_giftValueWidget = nullptr;
    QLabel* m_hpValueWidget = nullptr;
    QLabel* m_staminaValueWidget = nullptr;
    QLabel* m_bleedResValueWidget = nullptr;
    QLabel* m_poisonResValueWidget = nullptr;
    QLabel* m_curseResValueWidget = nullptr;
};

class CharacterInfoWidget: public QWidget {
    Q_OBJECT
public:
    explicit CharacterInfoWidget(QWidget* parent);
    void setCharacter(const fssm::parse::dsr::DSRCharacterInfo* charInfo);
private:
    CharacterStatusWidget* m_statusWidget = nullptr;
};
}
