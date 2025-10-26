#include "CharInfo.h"

#include <iostream>
#include <ostream>

#include "../Utils.h"

namespace {
    struct WidgetsHelper {
        QLabel* labelW;
        QLabel* valueW;
        PixmapLabel* iconW;
    };
}

CharacterStatusWidget::CharacterStatusWidget(QWidget* parent): QWidget(parent) {
    QWidget* attributesWidget = new QWidget(this);

    m_nameValueWidget = new QLabel("Name", attributesWidget);
    m_nameValueWidget->setAlignment(Qt::AlignCenter);

    QLabel* covenantLabelWidget = new QLabel("Covenant", attributesWidget);
    m_covenantValueWidget = new QLabel(attributesWidget);

    PixmapLabel* levelIconWidget = new PixmapLabel(QPixmap(":/dsr_images/attr_level.png"), attributesWidget);
    QLabel* levelLabelWidget = new QLabel("Level", attributesWidget);
    m_levelValueWidget = new QLabel("Level", attributesWidget);

    PixmapLabel* soulsIconWidget = new PixmapLabel(QPixmap(":/dsr_images/attr_souls.png"), attributesWidget);
    QLabel* soulsLabelWidget = new QLabel("Souls", attributesWidget);
    m_soulsValueWidget = new QLabel(attributesWidget);

    PixmapLabel* vitalityIconWidget = new PixmapLabel(QPixmap(":/dsr_images/attr_vitality.png"), attributesWidget);
    QLabel* vitalityLabelWidget = new QLabel("Vitality", attributesWidget);
    m_vitalityValueWidget = new QLabel(attributesWidget);

    PixmapLabel* attunementIconWidget = new PixmapLabel(QPixmap(":/dsr_images/attr_attunement.png"), attributesWidget);
    QLabel* attunementLabelWidget = new QLabel("Attunement", attributesWidget);
    m_attunementValueWidget = new QLabel(attributesWidget);

    PixmapLabel* enduranceIconWidget = new PixmapLabel(QPixmap(":/dsr_images/attr_endurance.png"), attributesWidget);
    QLabel* enduranceLabelWidget = new QLabel("Endurance", attributesWidget);
    m_enduranceValueWidget = new QLabel(attributesWidget);

    PixmapLabel* strengthIconWidget = new PixmapLabel(QPixmap(":/dsr_images/attr_strength.png"), attributesWidget);
    QLabel* strengthLabelWidget = new QLabel("Strength", attributesWidget);
    m_strengthValueWidget = new QLabel(attributesWidget);

    PixmapLabel* dexterityIconWidget = new PixmapLabel(QPixmap(":/dsr_images/attr_dexterity.png"), attributesWidget);
    QLabel* dexterityLabelWidget = new QLabel("Dexterity", attributesWidget);
    m_dexterityValueWidget = new QLabel(attributesWidget);

    PixmapLabel* resistanceIconWidget = new PixmapLabel(QPixmap(":/dsr_images/attr_resistance.png"), attributesWidget);
    QLabel* resistanceLabelWidget = new QLabel("Resistance", attributesWidget);
    m_resistanceValueWidget = new QLabel(attributesWidget);

    PixmapLabel* intelligenceIconWidget = new PixmapLabel(QPixmap(":/dsr_images/attr_intelligence.png"), attributesWidget);
    QLabel* intelligenceLabelWidget = new QLabel("Intelligence", attributesWidget);
    m_intelligenceValueWidget = new QLabel(attributesWidget);

    PixmapLabel* faithIconWidget = new PixmapLabel(QPixmap(":/dsr_images/attr_faith.png"), attributesWidget);
    QLabel* faithLabelWidget = new QLabel("Faith", attributesWidget);
    m_faithValueWidget = new QLabel(attributesWidget);

    PixmapLabel* humanityIconWidget =new PixmapLabel(QPixmap(":/dsr_images/attr_humanity.png"), attributesWidget);
    QLabel* humanityLabelWidget = new QLabel("Humanity", attributesWidget);
    m_humanityValueWidget = new QLabel(attributesWidget);

    QGridLayout* attributesLayout = new QGridLayout(attributesWidget);
    attributesLayout->setContentsMargins(0, 0, 0, 0);

    attributesLayout->addWidget(m_nameValueWidget, 0, 0, 1, 3);

    for (auto [labelW, valueW, iconW] : std::initializer_list<WidgetsHelper> {
        {covenantLabelWidget, m_covenantValueWidget, nullptr},
        {levelLabelWidget, m_levelValueWidget, levelIconWidget},
        {soulsLabelWidget, m_soulsValueWidget, soulsIconWidget},
        {vitalityLabelWidget, m_vitalityValueWidget, vitalityIconWidget},
        {attunementLabelWidget, m_attunementValueWidget, attunementIconWidget},
        {enduranceLabelWidget, m_enduranceValueWidget, enduranceIconWidget},
        {strengthLabelWidget, m_strengthValueWidget, strengthIconWidget},
        {dexterityLabelWidget, m_dexterityValueWidget, dexterityIconWidget},
        {resistanceLabelWidget, m_resistanceValueWidget, resistanceIconWidget},
        {intelligenceLabelWidget, m_intelligenceValueWidget, intelligenceIconWidget},
        {faithLabelWidget, m_faithValueWidget, faithIconWidget},
        {humanityLabelWidget, m_humanityValueWidget, humanityIconWidget},
    }) {
        int row = attributesLayout->rowCount();
        labelW->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        valueW->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        if (iconW != nullptr) {
            attributesLayout->addWidget(iconW, row, 0);
        }
        attributesLayout->addWidget(labelW, row, 1, 1, (iconW == nullptr) ? 2 : 1);
        attributesLayout->addWidget(valueW, row, 2);
    }

    attributesLayout->setRowStretch(attributesLayout->rowCount(), 1);
    attributesLayout->setColumnStretch(0, 0);
    attributesLayout->setColumnStretch(1, 1);
    attributesLayout->setColumnStretch(2, 1);

    QWidget* statsWidget = new QWidget(this);

    QLabel* hollowLabel = new QLabel("State", statsWidget);
    m_hollowValueWidget = new QLabel(statsWidget);

    QLabel* sexLabel = new QLabel("Sex", statsWidget);
    m_sexValueWidget = new QLabel(statsWidget);

    QLabel* classLabel = new QLabel("Class", statsWidget);
    m_classValueWidget = new QLabel(statsWidget);

    QLabel* physiqueLabel = new QLabel("Physique", statsWidget);
    m_physiqueValueWidget = new QLabel(statsWidget);

    QLabel* giftLabel = new QLabel("Gift", statsWidget);
    m_giftValueWidget = new QLabel(statsWidget);

    PixmapLabel* hpIconWidget = new PixmapLabel(QPixmap(":/dsr_images/hp"), statsWidget);
    QLabel* hpLabel = new QLabel("HP", statsWidget);
    m_hpValueWidget = new QLabel(statsWidget);

    PixmapLabel* staminaIconWidget = new PixmapLabel(QPixmap(":/dsr_images/stamina"), statsWidget);
    QLabel* staminaLabel = new QLabel("Stamina", statsWidget);
    m_staminaValueWidget = new QLabel(statsWidget);

    PixmapLabel* bleed_resIconWidget = new PixmapLabel(QPixmap(":/dsr_images/res_bleed"), statsWidget);
    QLabel* bleedResLabel = new QLabel("Bleed Resistance", statsWidget);
    m_bleed_resValueWidget = new QLabel(statsWidget);

    PixmapLabel* poison_resIconWidget = new PixmapLabel(QPixmap(":/dsr_images/res_poison"), statsWidget);
    QLabel* poisonResLabel = new QLabel("Poison Resistance", statsWidget);
    m_poisonResValueWidget = new QLabel(statsWidget);

    PixmapLabel* curseResIconWidget = new PixmapLabel(QPixmap(":/dsr_images/res_curse"), statsWidget);
    QLabel* curseResLabel = new QLabel("Curse Resistance", statsWidget);
    m_curseResValueWidget = new QLabel(statsWidget);

    QGridLayout* statsLayout = new QGridLayout(statsWidget);
    statsLayout->setContentsMargins(0, 0, 0, 0);

    for (auto [labelW, valueW, iconW] : std::initializer_list<WidgetsHelper> {
        {hollowLabel, m_hollowValueWidget, nullptr},
        {sexLabel, m_sexValueWidget, nullptr},
        {classLabel, m_classValueWidget, nullptr},
        {physiqueLabel, m_physiqueValueWidget, nullptr},
        {giftLabel, m_giftValueWidget, nullptr},
        {hpLabel, m_hpValueWidget, hpIconWidget},
        {staminaLabel, m_staminaValueWidget, staminaIconWidget},
        {bleedResLabel, m_bleed_resValueWidget, bleed_resIconWidget},
        {poisonResLabel, m_poisonResValueWidget, poison_resIconWidget},
        {curseResLabel, m_curseResValueWidget, curseResIconWidget},
    }) {
        int row = statsLayout->rowCount();
        labelW->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        valueW->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        if (iconW != nullptr) {
            statsLayout->addWidget(iconW, row, 0);
        }
        statsLayout->addWidget(labelW, row, (iconW == nullptr) ? 0 : 1, 1, (iconW == nullptr) ? 2 : 1);
        statsLayout->addWidget(valueW, row, 2);
    }
    statsLayout->setRowStretch(attributesLayout->rowCount(), 1);
    statsLayout->setColumnStretch(0, 0);
    statsLayout->setColumnStretch(1, 1);
    statsLayout->setColumnStretch(2, 1);

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(attributesWidget, 1);
    layout->addWidget(statsWidget, 1);
}

void CharacterStatusWidget::setCharacter() {

};

void CharacterStatusWidget::setEmpty() {

};

CharacterInfoWidget::CharacterInfoWidget(QWidget* parent): QWidget(parent) {
    m_statusWidget = new CharacterStatusWidget(this);
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_statusWidget, 1);
}

void CharacterInfoWidget::setCharacter() {
    m_statusWidget->setCharacter();

};