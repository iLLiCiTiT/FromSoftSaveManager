#pragma once

#include <QStackedWidget>
#include "Controller.h"
#include "SideBarWidget.h"
#include "SettingsWidget.h"
#include "GameWidgets.h"

class MainWindow : public QWidget {
    Q_OBJECT
public:
    explicit MainWindow(Controller* controller, QWidget* parent = nullptr);

private:
    Controller* m_controller = nullptr;
    QStackedWidget* m_stack = nullptr;
    SideBarWidget* m_sideBar = nullptr;
    SettingsWidget* m_settingsWidget = nullptr;
    DSRWidget* m_dsrWidget = nullptr;
};
