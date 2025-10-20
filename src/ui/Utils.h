#pragma once

#include <qevent.h>
#include <QSpinBox>


class FocusSpinBox: public QSpinBox {
    Q_OBJECT
public:
    explicit FocusSpinBox(QWidget* parent);
    void wheelEvent(QWheelEvent *event) override ;
};
