#pragma once
#include <QWidget>
#include <utility>
#include "Controller.h"


class BaseGameWidget: public QWidget {
    Q_OBJECT
signals:
    void showBackupsRequested();
public:
    explicit BaseGameWidget(Controller* controller, QString saveId, QWidget* parent): QWidget(parent), m_controller(controller), m_saveId(std::move(saveId)) {};
    virtual void refresh() = 0;
private:
    QString m_saveId;
    Controller* m_controller;
};
