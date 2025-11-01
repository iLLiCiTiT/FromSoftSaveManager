#pragma once
#include "SekiroWidget.h"
#include "../BaseGameWidget.h"

SekiroWidget::SekiroWidget(Controller* controller, const QString& saveId, QWidget* parent)
    : BaseGameWidget(controller, saveId, parent)
{};

void SekiroWidget::refresh() {
    // TODO implement
};
