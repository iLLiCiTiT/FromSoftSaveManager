#pragma once

#include <QPushButton>

class SquareButton : public QPushButton {
    Q_OBJECT
public:
    explicit SquareButton(QWidget* parent = nullptr)
        : QPushButton(parent) {
        QSizePolicy sp = sizePolicy();
        sp.setVerticalPolicy(QSizePolicy::Minimum);
        sp.setHorizontalPolicy(QSizePolicy::Minimum);
        setSizePolicy(sp);
        m_idealWidth = -1;
    }

    void showEvent(QShowEvent *event) {
        m_idealWidth = height();
        updateGeometry();
    }

    void resizeEvent(QResizeEvent *event) {
        m_idealWidth = height();
        updateGeometry();
    }
    QSize sizeHint() {
        if (m_idealWidth < 0) return QPushButton::sizeHint();
        return QSize(m_idealWidth, m_idealWidth);
    }
private:
    int m_idealWidth;
};
