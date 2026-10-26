#pragma once

#include <QHash>
#include <QPixmap>
#include <QWidget>
#include <optional>

#include "../ScrollHelper.h"
#include "../../parse/DSR/SaveFile.h"

namespace fssm::ui::dsr {
// Covenants in a framed panel styled like the Status screen: each with its offering
// item, rank and offerings towards the next rank; the joined covenant on the burnt
// orange plate of the game's selection. Scrolls when the window is too short.
class CovenantsWidget: public QWidget {
    Q_OBJECT
public:
    explicit CovenantsWidget(QWidget* parent);
    void setCharacter(const fssm::parse::dsr::DSRCharacterInfo* charInfo);
protected:
    void paintEvent(QPaintEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;
private:
    QPixmap icon(const QString& resource, int size);

    std::optional<fssm::parse::dsr::DSRCharacterInfo> m_character;
    QHash<QString, QPixmap> m_iconCache;
    fssm::ui::PaintedScroll m_scroll {this};
};
}
