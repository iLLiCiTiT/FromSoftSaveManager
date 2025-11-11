#pragma once
#include "../BaseGameWidget.h"

class DS2Widget: public BaseGameWidget {
    Q_OBJECT
public:
    explicit DS2Widget(Controller* controller, const QString& saveId, QWidget* parent);
    void refresh() override;
protected:
    void paintEvent(QPaintEvent* event) override;
private:
    QPixmap m_bgPix;
};
