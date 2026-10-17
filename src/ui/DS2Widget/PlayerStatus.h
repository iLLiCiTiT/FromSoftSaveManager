#pragma once

#include <QHash>
#include <QPixmap>
#include <QWidget>

#include "../../parse/DS2/SaveFile.h"

namespace fssm::ui::ds2 {
// "Player Status" screen drawn like the game's: level, souls and soul memory on
// top, then the attributes and the values that come straight from the save
// (HP) or from the game's tables (souls to next level, equip load).
class PlayerStatusWidget: public QWidget {
    Q_OBJECT
public:
    explicit PlayerStatusWidget(QWidget* parent = nullptr);
    void setCharacter(const fssm::parse::ds2::DS2CharacterInfo* charInfo);
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    QPixmap icon(const QString& name, int size);
    void drawRow(QPainter& painter, const QRect& rect, const QString& iconName, const QString& label,
                 const QString& value);
    void drawSectionHeader(QPainter& painter, const QRect& rect, const QString& title);

    bool m_hasCharacter = false;
    fssm::parse::ds2::DS2CharacterInfo m_char;
    QPixmap m_window;
    QHash<QString, QPixmap> m_iconCache;
};
}
