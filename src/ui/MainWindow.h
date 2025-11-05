#pragma once

#include <QStackedWidget>
#include <QGraphicsBlurEffect>

#include "Controller.h"
#include "ManageBackupsWidget.h"
#include "SideBarWidget.h"
#include "SettingsWidget.h"
#include "BaseGameWidget.h"
#include "GameWidgets.h"

class MainWindow : public QWidget {
    Q_OBJECT
public:
    explicit MainWindow(Controller* controller, QWidget* parent = nullptr);
    void refresh();
protected:
    void showEvent(QShowEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
private slots:
    void onTabChange(const QString& saveId);
    void onSaveIdChange(const QString& saveId);
    void onPathsConfigChange();
    void onShowBackupsRequest();
    void onHideBackupsRequest();
    void onOpacityAnimChange(const QVariant& value);
    void onOpacityAnimFinish();
private:
    QString m_saveId;
    Controller* m_controller = nullptr;
    QStackedWidget* m_stack = nullptr;
    SideBarWidget* m_sideBar = nullptr;
    SettingsWidget* m_settingsWidget = nullptr;
    std::unordered_map<QString, BaseGameWidget*> m_widgetsMapping;
    ManageBackupsOverlayWidget* m_manageBackupsOverlay = nullptr;
    QGraphicsBlurEffect* m_blurEffect = nullptr;
    QGraphicsOpacityEffect* m_manageOpacityEffect = nullptr;
    QVariantAnimation* m_manageOpacityAnim = nullptr;
    void updateOverlayGeo();
};
