#include "CharInfo.h"

#include <QVBoxLayout>

namespace fssm::ui::er {
NameWidget::NameWidget(QWidget* parent): QWidget(parent) {
    setAttribute(Qt::WA_TranslucentBackground, true);

    m_nameLabel = new QLabel(this);
    m_nameLabel->setAttribute(Qt::WA_TranslucentBackground, true);

    m_sepWidget = new NameSepWidget(this);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(m_nameLabel, 0);
    layout->addWidget(m_sepWidget, 0);
}

ERAttributesLabel::ERAttributesLabel(QWidget* parent): QWidget(parent) {
    setAttribute(Qt::WA_TranslucentBackground, true);

    m_iconWidget = new PixmapLabel(QPixmap(":/er_images/attrs_icon"), this);
    m_iconWidget->setAttribute(Qt::WA_TranslucentBackground, true);

    m_labelWidget = new QLabel("Attribute Points", this);
    m_labelWidget->setAttribute(Qt::WA_TranslucentBackground, true);

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);
    layout->addWidget(m_iconWidget, 0);
    layout->addWidget(m_labelWidget, 1);
}

CharacterStatusWidget::CharacterStatusWidget(QWidget* parent): QWidget(parent) {
    setAttribute(Qt::WA_TranslucentBackground, true);

    QWidget* attributesWidget = new QWidget(this);
    attributesWidget->setAttribute(Qt::WA_TranslucentBackground, true);

    m_nameLabel = new NameWidget(attributesWidget);

    QWidget* spacer1 = new QWidget(attributesWidget);
    spacer1->setFixedHeight(14);
    spacer1->setAttribute(Qt::WA_TranslucentBackground, true);

    AttrSepWidget* infoSep = new AttrSepWidget(attributesWidget);

    m_levelValueW = new QLabel(attributesWidget);
    m_runesValueW = new QLabel(attributesWidget);

    QWidget* spacer2 = new QWidget(attributesWidget);
    spacer2->setFixedHeight(14);
    spacer2->setAttribute(Qt::WA_TranslucentBackground, true);

    ERAttributesLabel* attributesLabel = new ERAttributesLabel(attributesWidget);
    AttrSepWidget* attributesSep = new AttrSepWidget(attributesWidget);

    m_vigorValueW = new QLabel(attributesWidget);
    m_mindValueW = new QLabel(attributesWidget);
    m_enduranceValueW = new QLabel(attributesWidget);
    m_strengthValueW = new QLabel(attributesWidget);
    m_dexterityValueW = new QLabel(attributesWidget);
    m_intelligenceValueW = new QLabel(attributesWidget);
    m_faithValueW = new QLabel(attributesWidget);
    m_arcaneValueW = new QLabel(attributesWidget);

    QGridLayout* attributesLayout = new QGridLayout(attributesWidget);
    attributesLayout->setSpacing(2);
    attributesLayout->setContentsMargins(0, 0, 0, 0);
    attributesLayout->addWidget(m_nameLabel, 0, 0, 1, 3);
    attributesLayout->addWidget(spacer1, 1, 0);

    const auto addAttrsGridRow = [&](QString label, QLabel* valueW) {
        QLabel* labelW = new QLabel(label, attributesWidget);

        labelW->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        valueW->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

        labelW->setAttribute(Qt::WA_TranslucentBackground, true);
        valueW->setAttribute(Qt::WA_TranslucentBackground, true);

        int row = attributesLayout->rowCount();
        attributesLayout->addWidget(labelW, row, 1);
        attributesLayout->addWidget(valueW, row, 2);
    };

    int levelRow = attributesLayout->rowCount();
    addAttrsGridRow("Level", m_levelValueW);
    addAttrsGridRow("Runes", m_runesValueW);
    attributesLayout->addWidget(infoSep, levelRow, 0, 2, 1);
    attributesLayout->addWidget(spacer2, attributesLayout->rowCount(), 0);
    attributesLayout->addWidget(attributesLabel, attributesLayout->rowCount(), 0, 1 ,3);

    int attrsRow = attributesLayout->rowCount();
    addAttrsGridRow("Vigor", m_vigorValueW);
    addAttrsGridRow("Mind", m_mindValueW);
    addAttrsGridRow("Endurance", m_enduranceValueW);
    addAttrsGridRow("Strength", m_strengthValueW);
    addAttrsGridRow("Dexterity", m_dexterityValueW);
    addAttrsGridRow("Intelligence", m_intelligenceValueW);
    addAttrsGridRow("Faith", m_faithValueW);
    addAttrsGridRow("Arcane", m_arcaneValueW);
    attributesLayout->addWidget(attributesSep, attrsRow, 0, 8, 1);

    attributesLayout->setRowStretch(attributesLayout->rowCount(), 1);
    attributesLayout->setColumnStretch(0, 0);
    attributesLayout->setColumnStretch(1, 1);
    attributesLayout->setColumnStretch(2, 1);

    QWidget* statsWidget = new QWidget(this);
    statsWidget->setAttribute(Qt::WA_TranslucentBackground, true);

    m_hpValueW = new QLabel(statsWidget);
    m_fpValueW = new QLabel(statsWidget);
    m_staminaValueW = new QLabel(statsWidget);

    QGridLayout* statsLayout = new QGridLayout(statsWidget);
    statsLayout->setContentsMargins(0, 0, 0, 0);
    const auto addStatsGridRow = [&](QString label, QLabel* valueW) {
        QLabel* labelW = new QLabel(label, attributesWidget);

        labelW->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        valueW->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

        labelW->setAttribute(Qt::WA_TranslucentBackground, true);
        valueW->setAttribute(Qt::WA_TranslucentBackground, true);

        int row = statsLayout->rowCount();
        statsLayout->addWidget(labelW, row, 0);
        statsLayout->addWidget(valueW, row, 1);
    };

    addStatsGridRow("HP", m_hpValueW);
    addStatsGridRow("FP", m_fpValueW);
    addStatsGridRow("Stamina", m_staminaValueW);

    statsLayout->setRowStretch(statsLayout->rowCount(), 1);
    statsLayout->setColumnStretch(0, 0);
    statsLayout->setColumnStretch(1, 1);

    QHBoxLayout* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(attributesWidget, 1);
    mainLayout->addWidget(statsWidget, 1);

    setCharacter(nullptr);
}

void CharacterStatusWidget::setCharacter(const fssm::parse::er::ERCharacterInfo* charInfo) const {
    if (charInfo == nullptr) return setEmpty();
    QString runesValue = QString::number(charInfo->runes);
    runesValue.push_back(" (");
    runesValue.push_back(QString::number(charInfo->earnedRunes));
    runesValue.push_back(")");
    m_nameLabel->setText(QString::fromStdU16String(charInfo->name));
    m_levelValueW->setText(QString::number(charInfo->level));
    m_runesValueW->setText(runesValue);
    m_vigorValueW->setText(QString::number(charInfo->vigor));
    m_mindValueW->setText(QString::number(charInfo->mind));
    m_enduranceValueW->setText(QString::number(charInfo->endurance));
    m_strengthValueW->setText(QString::number(charInfo->strength));
    m_dexterityValueW->setText(QString::number(charInfo->dexterity));
    m_intelligenceValueW->setText(QString::number(charInfo->intelligence));
    m_faithValueW->setText(QString::number(charInfo->faith));
    m_arcaneValueW->setText(QString::number(charInfo->arcane));

    const auto concatValues = [](const uint32_t& currentV, const uint32_t& maxV) {
        QString value = QString::number(currentV);
        value.push_back("/");
        value.push_back(QString::number(maxV));
        return value;
    };

    m_hpValueW->setText(concatValues(charInfo->hpCurrent, charInfo->hpMax));
    m_fpValueW->setText(concatValues(charInfo->fpCurrent, charInfo->fpMax));
    m_staminaValueW->setText(concatValues(charInfo->staminaCurrent, charInfo->staminaMax));
}

void CharacterStatusWidget::setEmpty() const {
    m_nameLabel->setText("< Empty >");
    m_levelValueW->setText("");
    m_runesValueW->setText("");
    m_vigorValueW->setText("");
    m_mindValueW->setText("");
    m_enduranceValueW->setText("");
    m_strengthValueW->setText("");
    m_dexterityValueW->setText("");
    m_intelligenceValueW->setText("");
    m_faithValueW->setText("");
    m_arcaneValueW->setText("");
    m_hpValueW->setText("");
    m_fpValueW->setText("");
    m_staminaValueW->setText("");
}
};
