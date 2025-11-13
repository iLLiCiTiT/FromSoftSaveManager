#pragma once

#include <QLabel>
#include "../../parse/Parse.h"

namespace fssm::ui::ds3 {

class CharacterStatusWidget: public QWidget {
    Q_OBJECT
public:
    explicit CharacterStatusWidget(QWidget* parent);
    void setCharacter(const fssm::parse::ds3::DS3CharacterInfo* charInfo);
private:
    void setEmpty();
    QLabel* m_nameValueWidget = nullptr;
    QLabel* m_levelValueWidget = nullptr;
    QLabel* m_soulsValueWidget = nullptr;
    QLabel* m_vigorValueWidget = nullptr;
    QLabel* m_attunementValueWidget = nullptr;
    QLabel* m_enduranceValueWidget = nullptr;
    QLabel* m_vitalityValueWidget = nullptr;
    QLabel* m_strengthValueWidget = nullptr;
    QLabel* m_dexterityValueWidget = nullptr;
    QLabel* m_intelligenceValueWidget = nullptr;
    QLabel* m_faithValueWidget = nullptr;
    QLabel* m_luckValueWidget = nullptr;

    QLabel* m_hpValueWidget = nullptr;
    QLabel* m_fpValueWidget = nullptr;
    QLabel* m_staminaValueWidget = nullptr;
    QLabel* m_bleedResValueWidget = nullptr;
    QLabel* m_poisonResValueWidget = nullptr;
    QLabel* m_frostResValueWidget = nullptr;
    QLabel* m_curseResValueWidget = nullptr;
};

class CharacterInfoWidget: public QWidget {
    Q_OBJECT
public:
    explicit CharacterInfoWidget(QWidget* parent);
    void setCharacter(const fssm::parse::ds3::DS3CharacterInfo* charInfo);
private:
    CharacterStatusWidget* m_statusWidget = nullptr;
};
}