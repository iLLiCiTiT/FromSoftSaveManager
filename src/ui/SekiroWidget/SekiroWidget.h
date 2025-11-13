#pragma once
#include "../BaseGameWidget.h"

class SekiroWidget: public BaseGameWidget {
    Q_OBJECT
public:
    explicit SekiroWidget(Controller* controller, const QString& saveId, QWidget* parent);
    void refresh() override;
protected:
    void paintEvent(QPaintEvent* event) override;
private:
    QPixmap m_bgPix;
};
