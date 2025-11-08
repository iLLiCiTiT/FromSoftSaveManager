#pragma once
#include "../BaseGameWidget.h"

class ERWidget: public BaseGameWidget {
    Q_OBJECT
public:
    explicit ERWidget(Controller* controller, const QString& saveId, QWidget* parent);
    void refresh() override;
protected:
    void paintEvent(QPaintEvent* event) override;
};
