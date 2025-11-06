#include "CharInfo.h"

#include "../Utils.h"
#include "../../parse/Parse.h"

namespace fssm::ui::ds3 {
struct WidgetsHelper {
    QLabel* labelW;
    QLabel* valueW;
    PixmapLabel* iconW;
};

CharacterStatusWidget::CharacterStatusWidget(QWidget* parent): QWidget(parent) {
    setAttribute(Qt::WA_TranslucentBackground, true);

    QWidget* attributesWidget = new QWidget(this);
    attributesWidget->setAttribute(Qt::WA_TranslucentBackground, true);

    m_nameValueWidget = new QLabel("< Empty >", attributesWidget);
    m_nameValueWidget->setAlignment(Qt::AlignCenter);
    m_nameValueWidget->setAttribute(Qt::WA_TranslucentBackground, true);

    PixmapLabel* levelIconWidget = new PixmapLabel(QPixmap(":/ds3_images/attr_level"), attributesWidget);
    QLabel* levelLabelWidget = new QLabel("Level", attributesWidget);
    m_levelValueWidget = new QLabel(attributesWidget);

    PixmapLabel* soulsIconWidget = new PixmapLabel(QPixmap(":/ds3_images/attr_souls"), attributesWidget);
    QLabel* soulsLabelWidget = new QLabel("Souls", attributesWidget);
    m_soulsValueWidget = new QLabel(attributesWidget);

    PixmapLabel* vigorIconWidget = new PixmapLabel(QPixmap(":/ds3_images/attr_vigor"), attributesWidget);
    QLabel* vigorLabelWidget = new QLabel("Vitality", attributesWidget);
    m_vigorValueWidget = new QLabel(attributesWidget);

    PixmapLabel* attunementIconWidget = new PixmapLabel(QPixmap(":/ds3_images/attr_attunement"), attributesWidget);
    QLabel* attunementLabelWidget = new QLabel("Attunement", attributesWidget);
    m_attunementValueWidget = new QLabel(attributesWidget);

    PixmapLabel* enduranceIconWidget = new PixmapLabel(QPixmap(":/ds3_images/attr_endurance"), attributesWidget);
    QLabel* enduranceLabelWidget = new QLabel("Endurance", attributesWidget);
    m_enduranceValueWidget = new QLabel(attributesWidget);

    PixmapLabel* vitalityIconWidget = new PixmapLabel(QPixmap(":/ds3_images/attr_vitality"), attributesWidget);
    QLabel* vitalityLabelWidget = new QLabel("Vitality", attributesWidget);
    m_vitalityValueWidget = new QLabel(attributesWidget);

    PixmapLabel* strengthIconWidget = new PixmapLabel(QPixmap(":/ds3_images/attr_strength"), attributesWidget);
    QLabel* strengthLabelWidget = new QLabel("Strength", attributesWidget);
    m_strengthValueWidget = new QLabel(attributesWidget);

    PixmapLabel* dexterityIconWidget = new PixmapLabel(QPixmap(":/ds3_images/attr_dexterity"), attributesWidget);
    QLabel* dexterityLabelWidget = new QLabel("Dexterity", attributesWidget);
    m_dexterityValueWidget = new QLabel(attributesWidget);

    PixmapLabel* intelligenceIconWidget = new PixmapLabel(QPixmap(":/ds3_images/attr_intelligence"), attributesWidget);
    QLabel* intelligenceLabelWidget = new QLabel("Intelligence", attributesWidget);
    m_intelligenceValueWidget = new QLabel(attributesWidget);

    PixmapLabel* faithIconWidget = new PixmapLabel(QPixmap(":/ds3_images/attr_faith"), attributesWidget);
    QLabel* faithLabelWidget = new QLabel("Faith", attributesWidget);
    m_faithValueWidget = new QLabel(attributesWidget);

    PixmapLabel* luckIconWidget = new PixmapLabel(QPixmap(":/ds3_images/attr_luck"), attributesWidget);
    QLabel* luckLabelWidget = new QLabel("Luck>", attributesWidget);
    m_luckValueWidget = new QLabel(attributesWidget);

    QGridLayout* attributesLayout = new QGridLayout(attributesWidget);
    attributesLayout->setContentsMargins(0, 0, 0, 0);

    attributesLayout->addWidget(m_nameValueWidget, 0, 0, 1, 3);

    for (auto [labelW, valueW, iconW] : std::initializer_list<WidgetsHelper> {
        {levelLabelWidget, m_levelValueWidget, levelIconWidget},
        {soulsLabelWidget, m_soulsValueWidget, soulsIconWidget},
        {vigorLabelWidget, m_vigorValueWidget, vigorIconWidget},
        {attunementLabelWidget, m_attunementValueWidget, attunementIconWidget},
        {enduranceLabelWidget, m_enduranceValueWidget, enduranceIconWidget},
        {vitalityLabelWidget, m_vitalityValueWidget, vitalityIconWidget},
        {strengthLabelWidget, m_strengthValueWidget, strengthIconWidget},
        {dexterityLabelWidget, m_dexterityValueWidget, dexterityIconWidget},
        {intelligenceLabelWidget, m_intelligenceValueWidget, intelligenceIconWidget},
        {faithLabelWidget, m_faithValueWidget, faithIconWidget},
        {luckLabelWidget, m_luckValueWidget, luckIconWidget},
    }) {
        int row = attributesLayout->rowCount();
        labelW->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        valueW->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        labelW->setAttribute(Qt::WA_TranslucentBackground, true);
        valueW->setAttribute(Qt::WA_TranslucentBackground, true);
        if (iconW != nullptr) {
            iconW->setAttribute(Qt::WA_TranslucentBackground, true);
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
    statsWidget->setAttribute(Qt::WA_TranslucentBackground, true);

    PixmapLabel* hpIconWidget = new PixmapLabel(QPixmap(":/ds3_images/attr_hp"), statsWidget);
    QLabel* hpLabel = new QLabel("HP", statsWidget);
    m_hpValueWidget = new QLabel(statsWidget);

    PixmapLabel* fpIconWidget = new PixmapLabel(QPixmap(":/ds3_images/attr_fp"), statsWidget);
    QLabel* fpLabel = new QLabel("FP", statsWidget);
    m_fpValueWidget = new QLabel(statsWidget);

    PixmapLabel* staminaIconWidget = new PixmapLabel(QPixmap(":/ds3_images/attr_stamina"), statsWidget);
    QLabel* staminaLabel = new QLabel("Stamina", statsWidget);
    m_staminaValueWidget = new QLabel(statsWidget);

    PixmapLabel* bleedResIconWidget = new PixmapLabel(QPixmap(":/ds3_images/attr_bleed_res"), statsWidget);
    QLabel* bleedResLabel = new QLabel("Bleed", statsWidget);
    m_bleedResValueWidget = new QLabel(statsWidget);

    PixmapLabel* poisonResIconWidget = new PixmapLabel(QPixmap(":/ds3_images/attr_poison_res"), statsWidget);
    QLabel* poisonResLabel = new QLabel("Poison", statsWidget);
    m_poisonResValueWidget = new QLabel(statsWidget);

    PixmapLabel* frostResIconWidget = new PixmapLabel(QPixmap(":/ds3_images/attr_frost_res"), statsWidget);
    QLabel* frostResLabel = new QLabel("Frost", statsWidget);
    m_frostResValueWidget = new QLabel(statsWidget);

    PixmapLabel* curseResIconWidget = new PixmapLabel(QPixmap(":/ds3_images/attr_curse_res"), statsWidget);
    QLabel* curseResLabel = new QLabel("Curse", statsWidget);
    m_curseResValueWidget = new QLabel(statsWidget);

    QGridLayout* statsLayout = new QGridLayout(statsWidget);
    statsLayout->setContentsMargins(0, 0, 0, 0);

    for (auto [labelW, valueW, iconW] : std::initializer_list<WidgetsHelper> {
        {hpLabel, m_hpValueWidget, hpIconWidget},
        {fpLabel, m_fpValueWidget, fpIconWidget},
        {staminaLabel, m_staminaValueWidget, staminaIconWidget},
        {bleedResLabel, m_bleedResValueWidget, bleedResIconWidget},
        {poisonResLabel, m_poisonResValueWidget, poisonResIconWidget},
        {frostResLabel, m_frostResValueWidget, frostResIconWidget},
        {curseResLabel, m_curseResValueWidget, curseResIconWidget},
    }) {
        int row = statsLayout->rowCount();
        labelW->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        valueW->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        labelW->setAttribute(Qt::WA_TranslucentBackground, true);
        valueW->setAttribute(Qt::WA_TranslucentBackground, true);
        if (iconW != nullptr) {
            iconW->setAttribute(Qt::WA_TranslucentBackground, true);
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

void CharacterStatusWidget::setCharacter(const fssm::parse::ds3::DS3CharacterInfo* charInfo) {
    if (charInfo == nullptr) return setEmpty();

    m_nameValueWidget->setText(QString::fromStdU16String(charInfo->name));
    m_levelValueWidget->setText(QString::number(charInfo->level));
    m_soulsValueWidget->setText(QString::number(charInfo->souls));
    m_vigorValueWidget->setText(QString::number(charInfo->vigor));
    m_attunementValueWidget->setText(QString::number(charInfo->attunement));
    m_enduranceValueWidget->setText(QString::number(charInfo->endurance));
    m_vitalityValueWidget->setText(QString::number(charInfo->vitality));
    m_strengthValueWidget->setText(QString::number(charInfo->strength));
    m_dexterityValueWidget->setText(QString::number(charInfo->dexterity));
    m_intelligenceValueWidget->setText(QString::number(charInfo->intelligence));
    m_faithValueWidget->setText(QString::number(charInfo->faith));
    m_luckValueWidget->setText(QString::number(charInfo->luck));
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

void CharacterStatusWidget::setEmpty() {
    m_nameValueWidget->setText("< Empty >");
    m_levelValueWidget->setText("");
    m_soulsValueWidget->setText("");
    m_vigorValueWidget->setText("");
    m_attunementValueWidget->setText("");
    m_enduranceValueWidget->setText("");
    m_vitalityValueWidget->setText("");
    m_strengthValueWidget->setText("");
    m_dexterityValueWidget->setText("");
    m_intelligenceValueWidget->setText("");
    m_faithValueWidget->setText("");
    m_luckValueWidget->setText("");
    m_hpValueWidget->setText("");
    m_staminaValueWidget->setText("");
    m_bleedResValueWidget->setText("");
    m_poisonResValueWidget->setText("");
    m_curseResValueWidget->setText("");
}

CharacterInfoWidget::CharacterInfoWidget(QWidget* parent): QWidget(parent) {
    setAttribute(Qt::WA_TranslucentBackground, true);
    m_statusWidget = new CharacterStatusWidget(this);
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_statusWidget, 1);
}

void CharacterInfoWidget::setCharacter(const fssm::parse::ds3::DS3CharacterInfo* charInfo) {
    m_statusWidget->setCharacter(charInfo);
}
}