#pragma once

#include <QWidget>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QIcon>
#include "SquareButton.h"
#include "Controller.h"
#include "SideBar.h"

class MainWindow : public QWidget {
    Q_OBJECT
public:
    explicit MainWindow(Controller* controller, QWidget* parent = nullptr);

private:
    Controller* m_controller {nullptr};
    QStackedWidget* m_stack {nullptr};
    SideBar *m_sideBar{nullptr};
};
