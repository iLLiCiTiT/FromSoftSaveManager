#pragma once

#include <QListView>
#include <QStandardItemModel>
#include <vector>

#include "CharInfo.h"
#include "CharacterList.h"
#include "HeaderTabs.h"
#include "Inventory.h"
#include "WorldProgress.h"
#include "../BaseGameWidget.h"
#include "../Controller.h"
#include "../Utils.h"

namespace fssm::ui::ds3 {
class CharsListModel: public QStandardItemModel {
    Q_OBJECT
signals:
    void refreshed();
public:
    explicit CharsListModel(Controller* controller, const QString& saveId, QObject* parent);
    void refresh();
    const fssm::parse::ds3::DS3CharacterInfo* getCharByIdx(const int& index) const;
private:
    std::vector<fssm::parse::ds3::DS3CharacterInfo> m_chars;
    QString m_saveId;
    Controller* m_controller;
};
}

class DS3Widget: public BaseGameWidget {
    Q_OBJECT
public:
    explicit DS3Widget(Controller* controller, const QString& saveId, QWidget* parent);
    void refresh() override;
protected:
    void paintEvent(QPaintEvent* event) override;
private slots:
    void onRefresh();
    void onSelectionChange(const QItemSelection& selected, const QItemSelection& deselected);
private:
    void showCharacter(const QModelIndex& index);

    QPixmap m_bgPix;
    fssm::ui::ds3::CharsListModel* m_model;
    QListView* m_view;
    fssm::ui::ds3::HeaderTabs* m_charTabs;
    fssm::ui::ds3::InventoryWidget* m_inventoryWidget;
    fssm::ui::ds3::CharacterInfoWidget* m_charInfoWidget;
    fssm::ui::ds3::WorldProgressWidget* m_worldWidget;
};
