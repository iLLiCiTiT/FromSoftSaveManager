#pragma once
#include <QLabel>
#include "../../parse/Parse.h"

class CovenantsWidget: public QWidget {
    Q_OBJECT
public:
    explicit CovenantsWidget(QWidget* parent);
    void setCharacter(const fssm::parse::dsr::DSRCharacterInfo* charInfo);
private:
    QLabel* m_wosWidget;
    QLabel* m_dwWidget;
    QLabel* m_potdWidget;
    QLabel* m_glsWidget;
    QLabel* m_fhWidget;
    QLabel* m_dmbWidget;
    QLabel* m_csWidget;
};