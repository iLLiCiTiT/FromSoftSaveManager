#include "NiceCheckbox.h"

#include <iostream>
#include <ostream>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>


QColor getSteppedColor(QColor& c1, QColor& c2, float offsetRatio) {
    float redDif = c1.redF() - c2.redF();
    float greenDif = c1.greenF() - c2.greenF();
    float blueDif = c1.blueF() - c2.blueF();

    return QColor(
        (c2.redF() + (redDif * offsetRatio)) * 255,
        (c2.greenF() + (greenDif * offsetRatio)) * 255,
        (c2.blueF() + (blueDif * offsetRatio)) * 255
    );
}
QColor mergeColors(QColor& c1, QColor& c2) {
    float a = c2.alphaF();
    return QColor(
        floor((c1.redF() + (c2.redF() * a)) * 0.5),
        floor((c1.greenF() + (c2.greenF() * a)) * 0.5),
        floor((c1.blueF() + (c2.blueF() * a)) * 0.5),
        c1.alpha()
    );
}

NiceCheckbox::NiceCheckbox(QWidget* parent): QFrame(parent) {
    setAttribute(Qt::WA_TranslucentBackground, true);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    QColor disabledColor = QColor(33, 33, 33, 127);

    m_checkedBgColor = QColor(0x56a06f);
    m_checkedBgColorDisabled = mergeColors(
        m_checkedBgColor, disabledColor
    );
    // m_uncheckedBgColor = QColor(0x21252B);
    m_uncheckedBgColor = QColor(0x1E1F22);
    m_uncheckedBgColorDisabled = mergeColors(
        m_uncheckedBgColor, disabledColor
    );
    m_checkerColor = QColor(0xD3D8DE);
    m_checkerColorDisabled = mergeColors(
        m_checkerColor, disabledColor
    );
    m_checkerColorHovered = QColor(0xF0F2F5);

    m_animationTimer = new QTimer(this);

    connect(m_animationTimer, SIGNAL(timeout()), this, SLOT(onAnimationTimeout()));
}

void NiceCheckbox::setCheckState(Qt::CheckState state) {
    if (m_checkState == state) return;
    if (!isTristate() && state == Qt::PartiallyChecked) return;
    m_checkState = state;
    emit stateChanged(state);

    if (m_animationTimer->isActive()) {
        m_animationTimer->stop();
    }
    if (!isVisible() || !isEnabled()) {
        if (m_checkState == Qt::Checked) {
            m_currentStep = m_steps;
        } else if (m_checkState == Qt::Unchecked) {
            m_currentStep = 0;
        } else {
            m_currentStep = m_middleStep;
        }
        if (isVisible()) repaint();
        return;
    }
    m_animationTimer->start(6);
}

QSize NiceCheckbox::sizeHint() const {
    return minimumSizeHint();
}

QSize NiceCheckbox::minimumSizeHint() const {
    return QSize(28, 16);
}

void NiceCheckbox::mousePressEvent(QMouseEvent *event) {
    if (event->buttons() == Qt::LeftButton) {
        event->accept();
        m_mousePressed = true;
        repaint();
    }
}

void NiceCheckbox::mouseReleaseEvent(QMouseEvent *event) {
    if (!m_mousePressed || event->button() != Qt::LeftButton) return;
    m_mousePressed = false;
    if (rect().contains(event->pos())) {
        event->accept();
        setCheckState(nextCheckState());
        emit clicked();
    }
}

void NiceCheckbox::mouseMoveEvent(QMouseEvent *event) {
    if (!m_mousePressed) return;
    bool underMouse = (rect().contains(event->pos()));
    if (underMouse != m_underMouse) {
        m_underMouse = underMouse;
        repaint();
    }
}

void NiceCheckbox::enterEvent(QEnterEvent *event) {
    m_underMouse = true;
    if (isEnabled()) repaint();
}

void NiceCheckbox::leaveEvent(QEvent *event) {
    m_underMouse = false;
    if (isEnabled()) repaint();
}

void NiceCheckbox::onAnimationTimeout() {
    if (m_checkState == Qt::Checked) {
        if (m_currentStep == m_steps) {
            m_animationTimer->stop();
            return;
        }
        m_currentStep++;
    } else if (m_checkState == Qt::Unchecked) {
        if (m_currentStep == 0) {
            m_animationTimer->stop();
            return;
        }
        m_currentStep--;
    } else {
        if (m_currentStep == m_middleStep) {
            m_animationTimer->stop();
            return;
        }
        if (m_currentStep < m_middleStep) {
            m_currentStep++;
        } else {
            m_currentStep--;
        }
    }
    repaint();
}

void NiceCheckbox::paintEvent(QPaintEvent *event) {
    QRect frameRect = rect();
    if (frameRect.width() == 0 || frameRect.height() == 0) return;

    frameRect.setLeft(frameRect.left() + frameRect.width() % 2);
    frameRect.setTop(frameRect.top() + frameRect.height() % 2);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);

    QColor bgColor;
    if (!isEnabled()) {
        bgColor = (m_checkState == Qt::Checked) ? m_checkedBgColorDisabled : m_uncheckedBgColorDisabled;
    } else if (m_currentStep == m_steps) {
        bgColor = m_checkedBgColor;
    } else if (m_currentStep == 0) {
        bgColor = m_uncheckedBgColor;
    } else {
        bgColor = getSteppedColor(m_checkedBgColor, m_uncheckedBgColor, m_currentStep / (float)m_steps);
    }

    QRect checkboxRect = frameRect;

    QRect sliderRect = checkboxRect;
    int sliderOffset = ceil(std::min(sliderRect.width(), sliderRect.height())) * 0.08;
    if (sliderOffset < 1) sliderOffset = 1;
    sliderRect.adjust(sliderOffset, sliderOffset, -sliderOffset, -sliderOffset);

    int radius = floor(std::min(sliderRect.width(), sliderRect.height()) * 0.5);
    painter.setBrush(bgColor);
    painter.drawRoundedRect(sliderRect, radius, radius);

    int checkerSize = frameRect.height();
    int areaWidth = checkboxRect.width() - checkerSize;

    int offsetX = 0;
    if (m_currentStep != 0)
        offsetX = (areaWidth / static_cast<float>(m_steps)) * m_currentStep;

    int posX = checkboxRect.left() + offsetX;
    int posY = checkboxRect.top();
    QRect checkerRect = QRect(posX, posY, checkerSize, checkerSize);

    QColor checkerColor = m_checkerColor;
    if (!isEnabled()) {
        checkerColor = m_checkerColorDisabled;
    } else if (m_underMouse) {
        checkerColor = m_checkerColorHovered;
    }
    painter.setBrush(checkerColor);
    painter.drawEllipse(checkerRect);
}
