#pragma once

#include <QListView>
#include <QStandardItemModel>
#include <vector>

#include "CharacterList.h"
#include "Inventory.h"
#include "../BaseGameWidget.h"
#include "../Controller.h"

namespace fssm::ui::sekiro {
class CharsListModel: public QStandardItemModel {
    Q_OBJECT
signals:
    void refreshed();
public:
    explicit CharsListModel(Controller* controller, const QString& saveId, QObject* parent);
    void refresh();
    const fssm::parse::sekiro::SekiroCharacterInfo* getCharByIdx(const int& index) const;
private:
    std::vector<fssm::parse::sekiro::SekiroCharacterInfo> m_chars;
    QString m_saveId;
    Controller* m_controller;
};
}

class SekiroWidget: public BaseGameWidget {
    Q_OBJECT
public:
    explicit SekiroWidget(Controller* controller, const QString& saveId, QWidget* parent);
    void refresh() override;
protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
private slots:
    void onRefresh();
    void onSelectionChange(const QItemSelection& selected, const QItemSelection& deselected);
private:
    void showCharacter(const QModelIndex& index);
    double frameScale() const;

    QPixmap m_bgPix;
    fssm::ui::sekiro::CharsListModel* m_model;
    QListView* m_view;
    fssm::ui::sekiro::InventoryWidget* m_inventoryWidget;
};
