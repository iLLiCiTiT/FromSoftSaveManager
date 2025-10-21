#pragma once

#include <qevent.h>
#include <QSpinBox>
#include <QPushButton>
#include <QVBoxLayout>

// Spinbox that does listen to wheel evens only if has focus
class FocusSpinBox: public QSpinBox {
    Q_OBJECT
public:
    explicit FocusSpinBox(QWidget* parent);
    void wheelEvent(QWheelEvent *event) override ;
};

// Custom tab widget
class TabButton: public QPushButton {
    Q_OBJECT
signals:
    void requested(int tabIdx);
public:
    explicit TabButton(int tabIdx, const QString& tabLabel, QWidget* parent);
    void setSelected(bool selected);
private slots:
    void onClick();
private:
    int m_tabIdx;
    bool m_isSelected = false;
};

struct TabButtonWrap {
    QWidget* widget = nullptr;
    TabButton* btn = nullptr;
};

class TabWidget: public QWidget {
    Q_OBJECT
public:
    explicit TabWidget(QWidget* parent);
    void addTab(const QString& tabLabel, QWidget* widget);

private slots:
    void setCurrentTab(int tabIdx);

private:
    std::vector<TabButtonWrap> m_tabs;
    int m_currentTabId = -1;
    QWidget* m_barWidget = nullptr;
    QHBoxLayout* m_barLayout = nullptr;
    QWidget* m_contentWidget = nullptr;
    QVBoxLayout* m_contentLayout = nullptr;
    QWidget* m_emptyWidget = nullptr;
};