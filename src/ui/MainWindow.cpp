#include "MainWindow.h"


MainWindow::MainWindow(Controller* controller, QWidget* parent)
    : QWidget(parent), m_controller(controller) {
    setWindowTitle("FromSoftSaveManager");
    resize(1100, 700);

    m_sideBar = new SideBarWidget(this);

    m_stack = new QStackedWidget(this);

    m_settingsWidget = new SettingsWidget(m_stack);

    m_stack->addWidget(m_settingsWidget);

    m_stack->setCurrentWidget(m_settingsWidget);

    QHBoxLayout* rootLayout = new QHBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);
    rootLayout->addWidget(m_sideBar, 0);
    rootLayout->addWidget(m_stack, 1);
}
