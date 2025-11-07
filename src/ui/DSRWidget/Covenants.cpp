#include "Covenants.h"
#include <QGridLayout>

namespace fssm::ui::dsr {
CovenantsWidget::CovenantsWidget(QWidget* parent): QWidget(parent) {
    setAttribute(Qt::WA_TranslucentBackground, true);

    QWidget* wrapperWidget = new QWidget(this);
    wrapperWidget->setAttribute(Qt::WA_TranslucentBackground, true);

    QLabel* headerWidget = new QLabel("Covenant levels", wrapperWidget);
    headerWidget->setAlignment(Qt::AlignCenter);
    headerWidget->setAttribute(Qt::WA_TranslucentBackground, true);

    QLabel* wosLabel = new QLabel("Warrior of Sunlight", wrapperWidget);
    m_wosWidget = new QLabel("0", wrapperWidget);
    QLabel* dwLabel = new QLabel("Darkwraith", wrapperWidget);
    m_dwWidget = new QLabel("0", wrapperWidget);
    QLabel* potdLabel = new QLabel("Path of the Dragon", wrapperWidget);
    m_potdWidget = new QLabel("0", wrapperWidget);
    QLabel* glsLabel = new QLabel("Gravelord Servant", wrapperWidget);
    m_glsWidget = new QLabel("0", wrapperWidget);
    QLabel* fhLabel = new QLabel("Forest Hunter", wrapperWidget);
    m_fhWidget = new QLabel("0", wrapperWidget);
    QLabel* dmbLabel = new QLabel("Darkmoon Blade", wrapperWidget);
    m_dmbWidget = new QLabel("0", wrapperWidget);
    QLabel* csLabel = new QLabel("Chaos Servant", wrapperWidget);
    m_csWidget = new QLabel("0", wrapperWidget);

    QGridLayout* wrapperLayout = new QGridLayout(wrapperWidget);
    wrapperLayout->setContentsMargins(0, 0, 0, 0);
    wrapperLayout->addWidget(headerWidget, 0, 0, 1, 2);

    const auto addGridRow = [&](QLabel* labelW, QLabel* valueW) {
        int row = wrapperLayout->rowCount();
        labelW->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        valueW->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        labelW->setAttribute(Qt::WA_TranslucentBackground, true);
        valueW->setAttribute(Qt::WA_TranslucentBackground, true);
        wrapperLayout->addWidget(labelW, row, 0);
        wrapperLayout->addWidget(valueW, row, 1);
    };

    addGridRow(wosLabel, m_wosWidget);
    addGridRow(dwLabel, m_dwWidget);
    addGridRow(potdLabel, m_potdWidget);
    addGridRow(glsLabel, m_glsWidget);
    addGridRow(fhLabel, m_fhWidget);
    addGridRow(dmbLabel, m_dmbWidget);
    addGridRow(csLabel, m_csWidget);

    int row = wrapperLayout->rowCount();
    wrapperLayout->setRowStretch(row, 1);

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(wrapperWidget, 1);
    layout->addStretch(1);
}

void CovenantsWidget::setCharacter(const fssm::parse::dsr::DSRCharacterInfo* charInfo) {
    const auto setCovenant = [&](QLabel* valueW, int idx) {
        QString value = "0";
        if (charInfo != nullptr) value = QString::number(charInfo->covenantLevels[idx]);
        valueW->setText(value);
    };
    // 3 covenants don't have a way how to level up
    setCovenant(m_wosWidget, 3);
    setCovenant(m_dwWidget, 4);
    setCovenant(m_potdWidget, 5);
    setCovenant(m_glsWidget, 6);
    setCovenant(m_fhWidget, 7);
    setCovenant(m_dmbWidget, 8);
    setCovenant(m_csWidget, 9);
}
}
