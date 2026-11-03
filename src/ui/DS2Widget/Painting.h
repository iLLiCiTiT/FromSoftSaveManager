#pragma once
// Drawing helpers shared by the DS2 views, which imitate the game's menus.

#include <QFont>
#include <QFontMetrics>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <algorithm>
#include <cmath>

namespace fssm::ui::ds2 {
    inline const QColor TEXT_COLOR(236, 232, 222);
    inline const QColor DIM_TEXT_COLOR(160, 150, 130);

    // The game's menus use a serif face; Georgia is the closest common one on Windows.
    inline QFont serifFont(int pixelSize) {
        QFont font("Georgia");
        font.setStyleHint(QFont::Serif);
        font.setPixelSize(std::max(8, pixelSize));
        return font;
    }

    // White text with a dark outline, like the counts and labels in the game.
    inline void drawOutlinedText(QPainter& painter, const QRect& rect, int flags,
                                 const QString& text, const QColor& color) {
        const QFontMetrics fm(painter.font());
        const QString elided = fm.elidedText(text, Qt::ElideRight, rect.width());
        const QRect bounds = fm.boundingRect(rect, flags, elided);
        QPainterPath path;
        path.addText(bounds.left(), bounds.top() + fm.ascent(), painter.font(), elided);
        painter.save();
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(QColor(0, 0, 0, 210), std::max(2.0, painter.font().pixelSize() / 7.0),
                            Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawPath(path);
        painter.setPen(Qt::NoPen);
        painter.setBrush(color);
        painter.drawPath(path);
        painter.restore();
    }

    // Draws `pix` into `target`, keeping its borders (in source pixels) from
    // being stretched other than by `scale`.
    inline void drawNineSlice(QPainter& painter, const QPixmap& pix, const QRect& target,
                              int left, int top, int right, int bottom, double scale) {
        const int sw = pix.width();
        const int sh = pix.height();
        const int sx[4] = {0, left, sw - right, sw};
        const int sy[4] = {0, top, sh - bottom, sh};
        const int dx[4] = {target.left(), target.left() + int(std::round(left * scale)),
                           target.right() + 1 - int(std::round(right * scale)), target.right() + 1};
        const int dy[4] = {target.top(), target.top() + int(std::round(top * scale)),
                           target.bottom() + 1 - int(std::round(bottom * scale)), target.bottom() + 1};
        for (int row = 0; row < 3; ++row) {
            for (int col = 0; col < 3; ++col) {
                const QRect src(sx[col], sy[row], sx[col + 1] - sx[col], sy[row + 1] - sy[row]);
                const QRect dst(dx[col], dy[row], dx[col + 1] - dx[col], dy[row + 1] - dy[row]);
                if (!src.isEmpty() && !dst.isEmpty()) painter.drawPixmap(dst, pix, src);
            }
        }
    }
}
