#pragma once

#include <QListView>
#include <QStandardItemModel>
#include <vector>

#include "CharacterList.h"
#include "HeaderTabs.h"
#include "Inventory.h"
#include "PlayerStatus.h"
#include "WorldProgress.h"
#include "../BaseGameWidget.h"
#include "../Controller.h"
#include "../Utils.h"

namespace fssm::ui::ds2 {
class CharsListModel: public QStandardItemModel {
    Q_OBJECT
signals:
    void refreshed();
public:
    explicit CharsListModel(Controller* controller, const QString& saveId, QObject* parent);
    void refresh();
    const fssm::parse::ds2::DS2CharacterInfo* getCharByIdx(const int& index) const;
private:
    std::vector<fssm::parse::ds2::DS2CharacterInfo> m_chars;
    QString m_saveId;
    Controller* m_controller;
};
}

class DS2Widget: public BaseGameWidget {
    Q_OBJECT
public:
    explicit DS2Widget(Controller* controller, const QString& saveId, QWidget* parent);
    void refresh() override;
protected:
    void paintEvent(QPaintEvent* event) override;
private slots:
    void onRefresh();
    void onSelectionChange(const QItemSelection& selected, const QItemSelection& deselected);
private:
    void showCharacter(const QModelIndex& index);

    QPixmap m_bgPix;
    fssm::ui::ds2::CharsListModel* m_model;
    QListView* m_view;
    fssm::ui::ds2::HeaderTabs* m_charTabs;
    fssm::ui::ds2::PlayerStatusWidget* m_statusWidget;
    fssm::ui::ds2::InventoryWidget* m_inventoryWidget;
    fssm::ui::ds2::WorldProgressWidget* m_worldWidget;
    fssm::ui::ds2::WorldProgressWidget* m_bonfiresWidget;
};
