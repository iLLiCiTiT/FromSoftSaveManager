#pragma once
#include <QLabel>
#include "SquareButton.h"

class TabButtonHint: public QWidget {
    Q_OBJECT
public:
    explicit TabButtonHint(const QString& title, QWidget* parent);
    ~TabButtonHint() override = default;

private:
    QLabel* m_labelWidget;
};

class TabIconButton: public SquareButton {
    Q_OBJECT
public:
    explicit TabIconButton(QIcon& icon, const QString& title, QWidget* parent);
    ~TabIconButton() override = default;
    void setSelected(bool selected);
    bool isSelected() const;
    virtual void enterEvent(QEnterEvent *event) override;
    virtual void leaveEvent(QEvent *event) override;

private:
    bool m_isSelected = false;
    TabButtonHint* m_hint = nullptr;
};

class SideBarWidget: public QWidget {
    Q_OBJECT
public:
    explicit SideBarWidget(QWidget* parent);

private:
    std::vector<TabIconButton*> m_tabs;
};