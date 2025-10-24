#pragma once
#include <QLabel>

class CovenantsWidget: public QWidget {
    Q_OBJECT
public:
    explicit CovenantsWidget(QWidget* parent);
    void setCharacter();
private:
    QLabel* m_wosWidget;
    QLabel* m_dwWidget;
    QLabel* m_potdWidget;
    QLabel* m_glsWidget;
    QLabel* m_fhWidget;
    QLabel* m_dmbWidget;
    QLabel* m_csWidget;
};