#pragma once
#include <QFrame>
#include <QTimer>

class NiceCheckbox: public QFrame {
    Q_OBJECT
signals:
    void stateChanged(Qt::CheckState state);
    void clicked();
public:
    explicit NiceCheckbox(QWidget* parent);
    Qt::CheckState checkState() { return m_checkState; }
    bool checked() const { return m_checkState == Qt::Checked; }
    bool isChecked() const { return checked(); }
    bool isTristate() const { return m_isTristate;}
    Qt::CheckState nextCheckState() {
        switch (m_checkState) {
            case Qt::PartiallyChecked:
                return Qt::Checked;
            case Qt::Checked:
                return Qt::Unchecked;
            case Qt::Unchecked:
                if (m_isTristate)
                    return Qt::PartiallyChecked;
                return Qt::Checked;
            default:
                return Qt::Checked;
        }
    }
public slots:
    void setCheckState(Qt::CheckState state);
    void setChecked(bool v) {
        if (checked() == v) return;
        setCheckState(v ? Qt::Checked : Qt::Unchecked);
    }
    void setTristate(bool tristate) {
        if (m_isTristate == tristate) return;
        m_isTristate = tristate;
        if (!m_isTristate && m_checkState == Qt::PartiallyChecked)
            setCheckState(Qt::Checked);
    }
protected:
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
private slots:
    void onAnimationTimeout();
private:
    Qt::CheckState m_checkState = Qt::Unchecked;
    bool m_isTristate = false;
    bool m_mousePressed = false;
    bool m_underMouse = false;
    int m_steps = 21;
    int m_middleStep = 11;
    int m_currentStep = 0;
    QTimer* m_animationTimer = nullptr;
    QColor m_checkedBgColor;
    QColor m_checkedBgColorDisabled;
    QColor m_uncheckedBgColor;
    QColor m_uncheckedBgColorDisabled;
    QColor m_checkerColor;
    QColor m_checkerColorDisabled;
    QColor m_checkerColorHovered;
};