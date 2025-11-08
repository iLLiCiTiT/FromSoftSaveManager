#include "CharInfo.h"

#include "../Utils.h"
#include "../../parse/Parse.h"

namespace fssm::ui::dsr {

CharacterStatusWidget::CharacterStatusWidget(QWidget* parent): QWidget(parent) {
    setAttribute(Qt::WA_TranslucentBackground, true);

    QWidget* attributesWidget = new QWidget(this);
    attributesWidget->setAttribute(Qt::WA_TranslucentBackground, true);

    m_nameValueWidget = new QLabel("< Empty >", attributesWidget);
    m_nameValueWidget->setAlignment(Qt::AlignCenter);
    m_nameValueWidget->setAttribute(Qt::WA_TranslucentBackground, true);

    QLabel* covenantLabelWidget = new QLabel("Covenant", attributesWidget);
    m_covenantValueWidget = new QLabel(attributesWidget);

    PixmapLabel* levelIconWidget = new PixmapLabel(QPixmap(":/dsr_images/attr_level"), attributesWidget);
    QLabel* levelLabelWidget = new QLabel("Level", attributesWidget);
    m_levelValueWidget = new QLabel(attributesWidget);

    PixmapLabel* soulsIconWidget = new PixmapLabel(QPixmap(":/dsr_images/attr_souls"), attributesWidget);
    QLabel* soulsLabelWidget = new QLabel("Souls", attributesWidget);
    m_soulsValueWidget = new QLabel(attributesWidget);

    PixmapLabel* vitalityIconWidget = new PixmapLabel(QPixmap(":/dsr_images/attr_vitality"), attributesWidget);
    QLabel* vitalityLabelWidget = new QLabel("Vitality", attributesWidget);
    m_vitalityValueWidget = new QLabel(attributesWidget);

    PixmapLabel* attunementIconWidget = new PixmapLabel(QPixmap(":/dsr_images/attr_attunement"), attributesWidget);
    QLabel* attunementLabelWidget = new QLabel("Attunement", attributesWidget);
    m_attunementValueWidget = new QLabel(attributesWidget);

    PixmapLabel* enduranceIconWidget = new PixmapLabel(QPixmap(":/dsr_images/attr_endurance"), attributesWidget);
    QLabel* enduranceLabelWidget = new QLabel("Endurance", attributesWidget);
    m_enduranceValueWidget = new QLabel(attributesWidget);

    PixmapLabel* strengthIconWidget = new PixmapLabel(QPixmap(":/dsr_images/attr_strength"), attributesWidget);
    QLabel* strengthLabelWidget = new QLabel("Strength", attributesWidget);
    m_strengthValueWidget = new QLabel(attributesWidget);

    PixmapLabel* dexterityIconWidget = new PixmapLabel(QPixmap(":/dsr_images/attr_dexterity"), attributesWidget);
    QLabel* dexterityLabelWidget = new QLabel("Dexterity", attributesWidget);
    m_dexterityValueWidget = new QLabel(attributesWidget);

    PixmapLabel* resistanceIconWidget = new PixmapLabel(QPixmap(":/dsr_images/attr_resistance"), attributesWidget);
    QLabel* resistanceLabelWidget = new QLabel("Resistance", attributesWidget);
    m_resistanceValueWidget = new QLabel(attributesWidget);

    PixmapLabel* intelligenceIconWidget = new PixmapLabel(QPixmap(":/dsr_images/attr_intelligence"), attributesWidget);
    QLabel* intelligenceLabelWidget = new QLabel("Intelligence", attributesWidget);
    m_intelligenceValueWidget = new QLabel(attributesWidget);

    PixmapLabel* faithIconWidget = new PixmapLabel(QPixmap(":/dsr_images/attr_faith"), attributesWidget);
    QLabel* faithLabelWidget = new QLabel("Faith", attributesWidget);
    m_faithValueWidget = new QLabel(attributesWidget);

    PixmapLabel* humanityIconWidget =new PixmapLabel(QPixmap(":/dsr_images/attr_humanity"), attributesWidget);
    QLabel* humanityLabelWidget = new QLabel("Humanity", attributesWidget);
    m_humanityValueWidget = new QLabel(attributesWidget);

    QGridLayout* attributesLayout = new QGridLayout(attributesWidget);
    attributesLayout->setContentsMargins(0, 0, 0, 0);

    attributesLayout->addWidget(m_nameValueWidget, 0, 0, 1, 3);

    const auto addGridRow = [](QGridLayout* layout, QLabel* labelW, QLabel* valueW, PixmapLabel* iconW = nullptr) {
        int row = layout->rowCount();
        labelW->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        valueW->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        labelW->setAttribute(Qt::WA_TranslucentBackground, true);
        valueW->setAttribute(Qt::WA_TranslucentBackground, true);
        if (iconW != nullptr) {
            iconW->setAttribute(Qt::WA_TranslucentBackground, true);
            layout->addWidget(iconW, row, 0);
        }
        layout->addWidget(labelW, row, 1, 1, (iconW == nullptr) ? 2 : 1);
        layout->addWidget(valueW, row, 2);
    };

    addGridRow(attributesLayout, covenantLabelWidget, m_covenantValueWidget);
    addGridRow(attributesLayout, levelLabelWidget, m_levelValueWidget, levelIconWidget);
    addGridRow(attributesLayout, soulsLabelWidget, m_soulsValueWidget, soulsIconWidget);
    addGridRow(attributesLayout, vitalityLabelWidget, m_vitalityValueWidget, vitalityIconWidget);
    addGridRow(attributesLayout, attunementLabelWidget, m_attunementValueWidget, attunementIconWidget);
    addGridRow(attributesLayout, enduranceLabelWidget, m_enduranceValueWidget, enduranceIconWidget);
    addGridRow(attributesLayout, strengthLabelWidget, m_strengthValueWidget, strengthIconWidget);
    addGridRow(attributesLayout, dexterityLabelWidget, m_dexterityValueWidget, dexterityIconWidget);
    addGridRow(attributesLayout, resistanceLabelWidget, m_resistanceValueWidget, resistanceIconWidget);
    addGridRow(attributesLayout, intelligenceLabelWidget, m_intelligenceValueWidget, intelligenceIconWidget);
    addGridRow(attributesLayout, faithLabelWidget, m_faithValueWidget, faithIconWidget);
    addGridRow(attributesLayout, humanityLabelWidget, m_humanityValueWidget, humanityIconWidget);

    attributesLayout->setRowStretch(attributesLayout->rowCount(), 1);
    attributesLayout->setColumnStretch(0, 0);
    attributesLayout->setColumnStretch(1, 1);
    attributesLayout->setColumnStretch(2, 1);

    QWidget* statsWidget = new QWidget(this);
    statsWidget->setAttribute(Qt::WA_TranslucentBackground, true);

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

    PixmapLabel* bleedResIconWidget = new PixmapLabel(QPixmap(":/dsr_images/res_bleed"), statsWidget);
    QLabel* bleedResLabel = new QLabel("Bleed Resistance", statsWidget);
    m_bleedResValueWidget = new QLabel(statsWidget);

    PixmapLabel* poisonResIconWidget = new PixmapLabel(QPixmap(":/dsr_images/res_poison"), statsWidget);
    QLabel* poisonResLabel = new QLabel("Poison Resistance", statsWidget);
    m_poisonResValueWidget = new QLabel(statsWidget);

    PixmapLabel* curseResIconWidget = new PixmapLabel(QPixmap(":/dsr_images/res_curse"), statsWidget);
    QLabel* curseResLabel = new QLabel("Curse Resistance", statsWidget);
    m_curseResValueWidget = new QLabel(statsWidget);

    QGridLayout* statsLayout = new QGridLayout(statsWidget);
    statsLayout->setContentsMargins(0, 0, 0, 0);

    addGridRow(statsLayout, hollowLabel, m_hollowValueWidget);
    addGridRow(statsLayout, sexLabel, m_sexValueWidget);
    addGridRow(statsLayout, classLabel, m_classValueWidget);
    addGridRow(statsLayout, physiqueLabel, m_physiqueValueWidget);
    addGridRow(statsLayout, giftLabel, m_giftValueWidget);
    addGridRow(statsLayout, hpLabel, m_hpValueWidget, hpIconWidget);
    addGridRow(statsLayout, staminaLabel, m_staminaValueWidget, staminaIconWidget);
    addGridRow(statsLayout, bleedResLabel, m_bleedResValueWidget, bleedResIconWidget);
    addGridRow(statsLayout, poisonResLabel, m_poisonResValueWidget, poisonResIconWidget);
    addGridRow(statsLayout, curseResLabel, m_curseResValueWidget, curseResIconWidget);

    statsLayout->setRowStretch(attributesLayout->rowCount(), 1);
    statsLayout->setColumnStretch(0, 0);
    statsLayout->setColumnStretch(1, 1);
    statsLayout->setColumnStretch(2, 1);

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(attributesWidget, 1);
    layout->addWidget(statsWidget, 1);
}

void CharacterStatusWidget::setCharacter(const fssm::parse::dsr::DSRCharacterInfo* charInfo) const {
    if (charInfo == nullptr) return setEmpty();

    QString covenentValue = QString::fromStdString(fssm::parse::dsr::DSR_COVENANT_LABELS[charInfo->covenantId].data());
    uint8_t covenantLevel = charInfo->covenantLevels[charInfo->covenantId];
    if (covenantLevel >= 30)
        covenentValue.push_back("+2");
    else if (covenantLevel >= 10)
        covenentValue.push_back("+1");

    m_nameValueWidget->setText(QString::fromStdU16String(charInfo->name));
    m_covenantValueWidget->setText(covenentValue);
    m_levelValueWidget->setText(QString::number(charInfo->level));
    m_soulsValueWidget->setText(QString::number(charInfo->souls));
    m_vitalityValueWidget->setText(QString::number(charInfo->vitality));
    m_attunementValueWidget->setText(QString::number(charInfo->attunement));
    m_enduranceValueWidget->setText(QString::number(charInfo->endurance));
    m_strengthValueWidget->setText(QString::number(charInfo->strength));
    m_dexterityValueWidget->setText(QString::number(charInfo->dexterity));
    m_resistanceValueWidget->setText(QString::number(charInfo->resistance));
    m_intelligenceValueWidget->setText(QString::number(charInfo->intelligence));
    m_faithValueWidget->setText(QString::number(charInfo->faith));
    m_humanityValueWidget->setText(QString::number(charInfo->humanity));
    m_hollowValueWidget->setText((charInfo->hollowState == 8) ? "Hollow" : "Human");
    m_sexValueWidget->setText((charInfo->sex == 1) ? "Male" : "Female");
    m_classValueWidget->setText(QString::fromStdString(fssm::parse::dsr::DSR_CLASSES[charInfo->classId].data()));
    m_physiqueValueWidget->setText(QString::fromStdString(fssm::parse::dsr::DSR_PHYSIQUE[charInfo->physiqueId].data()));
    m_giftValueWidget->setText(QString::fromStdString(fssm::parse::dsr::DSR_GIFTS[charInfo->giftId].data()));
    QString hpValue = QString::number(charInfo->hpCurrent);
    hpValue.append("/");
    hpValue.append(QString::number(charInfo->hpMax));
    QString staminaValue = QString::number(charInfo->staminaCurrent);
    staminaValue.append("/");
    staminaValue.append(QString::number(charInfo->staminaMax));
    m_hpValueWidget->setText(hpValue);
    m_staminaValueWidget->setText(staminaValue);
    m_bleedResValueWidget->setText(QString::number(charInfo->bleedRes));
    m_poisonResValueWidget->setText(QString::number(charInfo->poisonRes));
    m_curseResValueWidget->setText(QString::number(charInfo->curseRes));
}

void CharacterStatusWidget::setEmpty() const {
    m_nameValueWidget->setText("< Empty >");
    m_covenantValueWidget->setText("None");
    m_levelValueWidget->setText("");
    m_soulsValueWidget->setText("");
    m_vitalityValueWidget->setText("");
    m_attunementValueWidget->setText("");
    m_enduranceValueWidget->setText("");
    m_strengthValueWidget->setText("");
    m_dexterityValueWidget->setText("");
    m_resistanceValueWidget->setText("");
    m_intelligenceValueWidget->setText("");
    m_faithValueWidget->setText("");
    m_humanityValueWidget->setText("");
    m_hollowValueWidget->setText("");
    m_sexValueWidget->setText("");
    m_classValueWidget->setText("");
    m_physiqueValueWidget->setText("");
    m_giftValueWidget->setText("");
    m_hpValueWidget->setText("");
    m_staminaValueWidget->setText("");
    m_bleedResValueWidget->setText("");
    m_poisonResValueWidget->setText("");
    m_curseResValueWidget->setText("");
}

CharacterInfoWidget::CharacterInfoWidget(QWidget* parent): QWidget(parent) {
    m_statusWidget = new CharacterStatusWidget(this);
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_statusWidget, 1);
}

void CharacterInfoWidget::setCharacter(const fssm::parse::dsr::DSRCharacterInfo* charInfo) const {
    m_statusWidget->setCharacter(charInfo);
}
}
