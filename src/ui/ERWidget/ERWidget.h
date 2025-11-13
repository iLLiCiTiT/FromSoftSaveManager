#pragma once
#include <QListView>
#include <QStandardItemModel>

#include "CharInfo.h"
#include "../BaseGameWidget.h"

namespace fssm::ui::er {
const int CharIdRole = Qt::UserRole + 1;

class CharsListModel: public QStandardItemModel {
    Q_OBJECT
signals:
    void refreshed();
public:
    explicit CharsListModel(Controller* controller, const QString& saveId, QObject* parent);
    void refresh();
    const fssm::parse::er::ERCharacterInfo* getCharByIdx(const int& index) const;
private:
    std::vector<fssm::parse::er::ERCharacterInfo> m_chars;
    std::array<QStandardItem*, 10> m_items;
    QString m_saveId;
    Controller* m_controller;
};
}

class ERWidget: public BaseGameWidget {
    Q_OBJECT
public:
    explicit ERWidget(Controller* controller, const QString& saveId, QWidget* parent);
    void refresh() override;
protected:
    void paintEvent(QPaintEvent* event) override;
private slots:
    void onSelectionChange(const QItemSelection& selected, const QItemSelection& deselected);
    void onRefresh();
private:
    QListView* m_view = nullptr;
    TabWidget* m_charTabs = nullptr;
    fssm::ui::er::CharsListModel* m_model = nullptr;
    fssm::ui::er::CharacterInfoWidget* m_charInfoWidget = nullptr;
};
