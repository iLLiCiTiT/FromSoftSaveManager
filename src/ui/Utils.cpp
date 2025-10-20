#include "Utils.h"

FocusSpinBox::FocusSpinBox(QWidget* parent): QSpinBox(parent) {
    setFocusPolicy(Qt::StrongFocus);
};

void FocusSpinBox::wheelEvent(QWheelEvent *event) {
    if (hasFocus())
        QSpinBox::wheelEvent(event);
    else
        event->ignore();
};
