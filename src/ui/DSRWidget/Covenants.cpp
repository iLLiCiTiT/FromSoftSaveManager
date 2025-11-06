#include "Covenants.h"
#include <QGridLayout>

namespace fssm::ui::dsr {
struct WidgetsHelper {
    QLabel* labelW;
    QLabel* valueW;
};

CovenantsWidget::CovenantsWidget(QWidget* parent): QWidget(parent) {
    QWidget* wrapperWidget = new QWidget(this);

    QLabel* headerWidget = new QLabel("Covenant levels", wrapperWidget);
    headerWidget->setAlignment(Qt::AlignCenter);

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

    for (auto [labelW, valueW] : std::initializer_list<WidgetsHelper> {
        {wosLabel, m_wosWidget},
        {dwLabel, m_dwWidget},
        {potdLabel, m_potdWidget},
        {glsLabel, m_glsWidget},
        {fhLabel, m_fhWidget},
        {dmbLabel, m_dmbWidget},
        {csLabel, m_csWidget},
    }) {
        int row = wrapperLayout->rowCount();
        labelW->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        valueW->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        wrapperLayout->addWidget(labelW, row, 0);
        wrapperLayout->addWidget(valueW, row, 1);
    }
    int row = wrapperLayout->rowCount();
    wrapperLayout->setRowStretch(row, 1);

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(wrapperWidget, 1);
    layout->addStretch(1);
}

void CovenantsWidget::setCharacter(const fssm::parse::dsr::DSRCharacterInfo* charInfo) {
    // Skip 3 covenants that don't have a way how to level
    int idx = 3;
    for (auto valueW : std::initializer_list {
        m_wosWidget,
        m_dwWidget,
        m_potdWidget,
        m_glsWidget,
        m_fhWidget,
        m_dmbWidget,
        m_csWidget,
    }) {
        QString value = "0";
        if (charInfo != nullptr) value = QString::number(charInfo->covenantLevels[idx]);
        valueW->setText(value);
        idx += 1;
    }
}
}
