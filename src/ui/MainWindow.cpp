#include "MainWindow.h"


MainWindow::MainWindow(Controller* controller, QWidget* parent)
    : QWidget(parent), m_controller(controller) {
    setWindowTitle("FromSoftSaveManager");
    resize(960, 490);

    m_sideBar = new SideBarWidget(this);

    m_stack = new QStackedWidget(this);

    m_settingsWidget = new SettingsWidget(controller, m_stack);

    m_stack->addWidget(m_settingsWidget);

    m_saveId = "";
    m_stack->setCurrentWidget(m_settingsWidget);

    QHBoxLayout* rootLayout = new QHBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);
    rootLayout->addWidget(m_sideBar, 0);
    rootLayout->addWidget(m_stack, 1);

    connect(m_sideBar, SIGNAL(tabChanged(QString)), this, SLOT(onTabChange(QString)));
    connect(m_controller, SIGNAL(saveIdChanged(QString)), this, SLOT(onSaveIdChange(QString)));
    connect(m_controller, SIGNAL(pathsConfigChanged()), this, SLOT(onPathsConfigChange()));
}

void MainWindow::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    refresh();
}

void MainWindow::refresh() {
    std::unordered_set<QString> availableIds;
    QString firstId;
    bool wasEmpty = m_widgetsMapping.empty();
    for (const auto&[game, saveId, _savePath]: m_controller->getSaveFileItems()) {
        BaseGameWidget* gameWidget = nullptr;
        if (firstId.isEmpty()) firstId = saveId;
        availableIds.insert(saveId);
        if (m_widgetsMapping.find(saveId) != m_widgetsMapping.end()) {
            gameWidget = m_widgetsMapping.find(saveId)->second;
            gameWidget->refresh();
            continue;
        }
        switch (game) {
            case fsm::parse::Game::DSR:
                gameWidget = new DSRWidget(m_controller, saveId, m_stack);
                break;
            case fsm::parse::Game::DS2_SOTFS:
                gameWidget = new DS2Widget(m_controller, saveId, m_stack);
                break;

            case fsm::parse::Game::DS3:
                gameWidget = new DS3Widget(m_controller, saveId, m_stack);
                break;

            case fsm::parse::Game::Sekiro:
                gameWidget = new SekiroWidget(m_controller, saveId, m_stack);
                break;

            case fsm::parse::Game::ER:
                gameWidget = new ERWidget(m_controller, saveId, m_stack);
                break;

            default:
                break;
        }
        if (gameWidget != nullptr) {
            m_stack->addWidget(gameWidget);
            m_widgetsMapping[saveId] = gameWidget;
            m_sideBar->addTab(game, saveId);
            gameWidget->refresh();
        }
    }
    std::unordered_map<QString, BaseGameWidget*>::iterator it = m_widgetsMapping.begin();
    while (it != m_widgetsMapping.end()) {
        if (availableIds.find(it->first) != availableIds.end()) {
            it = std::next(it);
            continue;
        }

        it->second->setVisible(false);
        it->second->deleteLater();
        m_stack->removeWidget(it->second);
        m_sideBar->removeTab(it->first);
        it = m_widgetsMapping.erase(it);
    }

    // Jump to a game tab only if there were not any before
    if (!wasEmpty) return;
    if (m_widgetsMapping.find(m_saveId) == m_widgetsMapping.end()) {
        m_sideBar->setCurrentTab(firstId);
    }
}

void MainWindow::onTabChange(const QString& saveId) {
    if (m_saveId == saveId) return;
    // Empty save id is settings widget
    if (saveId == "") {
        m_saveId = "";
        m_stack->setCurrentWidget(m_settingsWidget);
        m_controller->setCurrentTabId(saveId);
        return;
    }

    // Save id not found
    if (m_widgetsMapping.find(saveId) == m_widgetsMapping.end()) return;
    // Discard changes if the current tab is settings
    if (m_saveId == "") m_settingsWidget->discardChanges();

    // Change visible widget
    m_stack->setCurrentWidget(m_widgetsMapping.find(saveId)->second);
    m_saveId = saveId;
    m_controller->setCurrentTabId(saveId);
}

void MainWindow::onSaveIdChange(const QString& saveId) {
    auto wIt = m_widgetsMapping.find(saveId);
    if (wIt != m_widgetsMapping.end()) wIt->second->refresh();
}

void MainWindow::onPathsConfigChange() {
    refresh();
}
