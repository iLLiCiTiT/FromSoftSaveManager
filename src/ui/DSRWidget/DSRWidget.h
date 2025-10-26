#pragma once

#include "CharInfo.h"
#include "Covenants.h"
#include "Inventory.h"
#include "../Controller.h"
#include "../Utils.h"
#include "../../parse/DSRSaveFile.h"

const int CHAR_ID_ROLE = Qt::UserRole + 1;
const int CHAR_NAME_ROLE = Qt::UserRole + 2;


class CharsListModel: public QStandardItemModel {
    Q_OBJECT
signals:
    void refreshed();
public:
    explicit CharsListModel(Controller* controller, const QString& saveId, QObject* parent);
    void refresh();
    fsm::parse::DSRCharacterInfo* getCharByIdx(const int& index);
private:
    std::vector<fsm::parse::DSRCharacterInfo> m_chars;
    std::array<QStandardItem*, 10> m_items;
    QString m_saveId;
    Controller* m_controller;
};


class DSRWidget: public QWidget {
    Q_OBJECT
public:
    explicit DSRWidget(Controller* controller, const QString& saveId, QWidget* parent);
    void refresh();
    void paintEvent(QPaintEvent* event) override;
private slots:
    void onRefresh();
    void onSelectionChange(const QItemSelection &selected, const QItemSelection &deselected);
private:
    QString m_saveId;
    QPixmap m_bgPix;
    Controller* m_controller;
    CharsListModel* m_model;
    QListView* m_view;
    TabWidget* m_charTabs;
    CovenantsWidget* m_covenantsWidget;
    CharacterInfoWidget* m_charInfoWidget;
    InventoryWidget* m_inventoryWidget;
};
