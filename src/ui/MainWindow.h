#pragma once

#include <QStackedWidget>
#include <QHBoxLayout>
#include "Controller.h"
#include "SideBarWidget.h"

class MainWindow : public QWidget {
    Q_OBJECT
public:
    explicit MainWindow(Controller* controller, QWidget* parent = nullptr);

private:
    Controller* m_controller {nullptr};
    QStackedWidget* m_stack {nullptr};
    SideBarWidget *m_sideBar{nullptr};
};
