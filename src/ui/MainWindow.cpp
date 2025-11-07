#include "MainWindow.h"

#include <iostream>


MainWindow::MainWindow(Controller* controller, QWidget* parent)
    : QWidget(parent), m_controller(controller) {
    setWindowTitle("FromSoftSaveManager");
    resize(960, 490);

    m_sideBar = new SideBarWidget(this);

    m_stack = new QStackedWidget(this);

    m_blurEffect = new QGraphicsBlurEffect(m_stack);
    m_blurEffect->setEnabled(false);
    m_blurEffect->setBlurHints(QGraphicsBlurEffect::QualityHint);
    m_blurEffect->setBlurRadius(0.0);

    m_stack->setGraphicsEffect(m_blurEffect);

    m_settingsWidget = new SettingsWidget(controller, m_stack);

    m_manageBackupsOverlay = new ManageBackupsOverlayWidget(controller, this);
    m_manageBackupsOverlay->setVisible(false);

    m_manageOpacityEffect = new QGraphicsOpacityEffect(m_manageBackupsOverlay);
    m_manageOpacityEffect->setEnabled(false);
    m_manageOpacityEffect->setOpacity(0.0);

    m_manageBackupsOverlay->setGraphicsEffect(m_manageOpacityEffect);

    m_manageOpacityAnim = new QVariantAnimation(m_manageBackupsOverlay);
    m_manageOpacityAnim->setEasingCurve(QEasingCurve::OutCubic);
    m_manageOpacityAnim->setDuration(200);
    m_manageOpacityAnim->setStartValue(0.0);
    m_manageOpacityAnim->setEndValue(1.0);

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
    connect(m_manageBackupsOverlay, SIGNAL(hideRequested()), this, SLOT(onHideBackupsRequest()));
    connect(m_manageOpacityAnim, SIGNAL(valueChanged(const QVariant&)), this, SLOT(onOpacityAnimChange(const QVariant&)));
    connect(m_manageOpacityAnim, SIGNAL(finished()), this, SLOT(onOpacityAnimFinish()));
}

void MainWindow::showEvent(QShowEvent *event) {
    refresh();
    updateOverlayGeo();
}

void MainWindow::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    updateOverlayGeo();
}

void MainWindow::refresh() {
    std::unordered_set<QString> availableIds;
    QString firstId;
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
            case fssm::Game::DSR:
                gameWidget = new DSRWidget(m_controller, saveId, m_stack);
                break;
            case fssm::Game::DS2_SOTFS:
                gameWidget = new DS2Widget(m_controller, saveId, m_stack);
                break;

            case fssm::Game::DS3:
                gameWidget = new DS3Widget(m_controller, saveId, m_stack);
                break;

            case fssm::Game::Sekiro:
                gameWidget = new SekiroWidget(m_controller, saveId, m_stack);
                break;

            case fssm::Game::ER:
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
            connect(gameWidget, SIGNAL(showBackupsRequested()), this, SLOT(onShowBackupsRequest()));
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

    QString saveId = m_controller->getLastSelectedSaveId();
    if (availableIds.find(saveId) == availableIds.end())
        saveId = firstId;

    if (saveId != m_saveId)
        m_sideBar->setCurrentTab(saveId);
}

void MainWindow::updateOverlayGeo() {
    if (m_manageBackupsOverlay->isVisible()) {
        m_manageBackupsOverlay->setGeometry(m_stack->geometry());
    }
}

void MainWindow::onTabChange(const QString& saveId) {
    if (m_saveId == saveId) return;
    // Empty save id is settings widget
    if (saveId == "") {
        m_saveId = "";
        m_stack->setCurrentWidget(m_settingsWidget);
        m_controller->setCurrentTabId(saveId);
        onHideBackupsRequest();
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

    if (m_saveId != "" && m_manageBackupsOverlay->isVisible())
        m_manageBackupsOverlay->refresh();
}

void MainWindow::onSaveIdChange(const QString& saveId) {
    auto wIt = m_widgetsMapping.find(saveId);
    if (wIt != m_widgetsMapping.end()) wIt->second->refresh();
}

void MainWindow::onPathsConfigChange() {
    refresh();
}

void MainWindow::onShowBackupsRequest() {
    m_blurEffect->setEnabled(true);
    m_manageOpacityEffect->setEnabled(true);
    m_manageBackupsOverlay->setVisible(true);
    if (m_manageOpacityAnim->direction() == QVariantAnimation::Backward)
        m_manageOpacityAnim->setDirection(QVariantAnimation::Forward);

    if (m_manageOpacityAnim->state() != QVariantAnimation::Running)
        m_manageOpacityAnim->start();

    updateOverlayGeo();

    m_manageBackupsOverlay->refresh();
}

void MainWindow::onHideBackupsRequest() {
    if (m_manageOpacityAnim->direction() == QVariantAnimation::Forward)
        m_manageOpacityAnim->setDirection(QVariantAnimation::Backward);

    if (m_manageOpacityAnim->state() != QVariantAnimation::Running)
        m_manageOpacityAnim->start();
    m_manageOpacityEffect->setEnabled(true);
}

void MainWindow::onOpacityAnimChange(const QVariant& value) {
    double dValue = value.toDouble();
    m_manageOpacityEffect->setOpacity(dValue);
    m_blurEffect->setBlurRadius(5.0 * dValue);
}

void MainWindow::onOpacityAnimFinish() {
    QVariant value = m_manageOpacityAnim->currentValue();
    onOpacityAnimChange(value);

    if (m_manageOpacityEffect->opacity() == 0.0) {
        // Overlay can be hidden and blur effect fully disabled
        m_manageBackupsOverlay->setVisible(false);
        m_blurEffect->setEnabled(false);
    } else {
        // Set last value of blue effect
        m_blurEffect->setBlurRadius(5.0);
    }
    // Always disable opacity effect on finished animation
    m_manageOpacityEffect->setEnabled(false);
}
