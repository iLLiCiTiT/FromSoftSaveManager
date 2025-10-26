#include "MainWindow.h"


MainWindow::MainWindow(Controller* controller, QWidget* parent)
    : QWidget(parent), m_controller(controller) {
    setWindowTitle("FromSoftSaveManager");
    resize(960, 490);

    m_sideBar = new SideBarWidget(this);

    m_stack = new QStackedWidget(this);

    m_settingsWidget = new SettingsWidget(m_stack);

    m_stack->addWidget(m_settingsWidget);

    m_saveId = "";
    m_stack->setCurrentWidget(m_settingsWidget);

    QHBoxLayout* rootLayout = new QHBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);
    rootLayout->addWidget(m_sideBar, 0);
    rootLayout->addWidget(m_stack, 1);

    connect(m_sideBar, SIGNAL(tabChanged(QString)), this, SLOT(onTabChange(QString)));
}

void MainWindow::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    refresh();
}

void MainWindow::refresh() {
    // TODO use controller to get all save information
    // - temporary for dev purposes
    QString saveId = "test";

    m_dsrWidget = new DSRWidget(m_controller, saveId, m_stack);
    m_stack->addWidget(m_dsrWidget);
    m_widgetsMapping[saveId] = m_dsrWidget;

    m_sideBar->addTab(fsm::parse::Game::DSR, saveId);
}

void MainWindow::onTabChange(QString saveId) {
    if (m_saveId == saveId) return;
    // Empty save id is settings widget
    if (saveId == "") {
        m_saveId = "";
        m_stack->setCurrentWidget(m_settingsWidget);
        return;
    }

    // Save id not found
    if (m_widgetsMapping.find(saveId) == m_widgetsMapping.end()) return;
    // Discard changes if the current tab is settings
    if (m_saveId == "") m_settingsWidget->discardChanges();

    // Change visible widget
    m_stack->setCurrentWidget(m_widgetsMapping.find(saveId)->second);
    m_saveId = saveId;
}
