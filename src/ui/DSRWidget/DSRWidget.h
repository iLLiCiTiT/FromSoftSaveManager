#pragma once

#include "CharInfo.h"
#include "Covenants.h"
#include "Inventory.h"
#include "../BaseGameWidget.h"
#include "../Controller.h"
#include "../Utils.h"
#include "../../parse/Parse.h"

namespace fssm::ui::dsr{
const int CHAR_ID_ROLE = Qt::UserRole + 1;
const int CHAR_NAME_ROLE = Qt::UserRole + 2;

class CharsListModel: public QStandardItemModel {
    Q_OBJECT
signals:
    void refreshed();
public:
    explicit CharsListModel(Controller* controller, const QString& saveId, QObject* parent);
    void refresh();
    fssm::parse::dsr::DSRCharacterInfo* getCharByIdx(const int& index);
private:
    std::vector<fssm::parse::dsr::DSRCharacterInfo> m_chars;
    std::array<QStandardItem*, 10> m_items;
    QString m_saveId;
    Controller* m_controller;
};
}

class DSRWidget: public BaseGameWidget {
    Q_OBJECT
public:
    explicit DSRWidget(Controller* controller, const QString& saveId, QWidget* parent);
    void refresh() override;
protected:
    void paintEvent(QPaintEvent* event) override;
private slots:
    void onRefresh();
    void onSelectionChange(const QItemSelection &selected, const QItemSelection &deselected);
private:
    QPixmap m_bgPix;
    fssm::ui::dsr::CharsListModel* m_model;
    QListView* m_view;
    TabWidget* m_charTabs;
    fssm::ui::dsr::CovenantsWidget* m_covenantsWidget;
    fssm::ui::dsr::CharacterInfoWidget* m_charInfoWidget;
    fssm::ui::dsr::InventoryWidget* m_inventoryWidget;
};
