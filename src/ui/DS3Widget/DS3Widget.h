#pragma once
#include "../BaseGameWidget.h"

class DS3Widget: public BaseGameWidget {
    Q_OBJECT
public:
    explicit DS3Widget(Controller* controller, const QString& saveId, QWidget* parent);
    void refresh() override;
};
