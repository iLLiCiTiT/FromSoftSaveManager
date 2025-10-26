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
    void refresh();
    void showEvent(QShowEvent *event) override;
private slots:
    void onTabChange(QString saveId);
private:
    QString m_saveId;
    Controller* m_controller = nullptr;
    QStackedWidget* m_stack = nullptr;
    SideBarWidget* m_sideBar = nullptr;
    SettingsWidget* m_settingsWidget = nullptr;
    DSRWidget* m_dsrWidget = nullptr;
    std::unordered_map<QString, QWidget*> m_widgetsMapping;
};
