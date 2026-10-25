#pragma once
// Drawing helpers shared by the DS3 views, which imitate the game's menus
// (dark stone panels, gold rules, serif text with a soft shadow).

#include <QFont>
#include <QHash>
#include <QFontMetrics>
#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <algorithm>
#include <cmath>

namespace fssm::ui::ds3 {
    inline const QColor TEXT_COLOR(226, 222, 212);
    inline const QColor DIM_TEXT_COLOR(150, 144, 132);
    inline const QColor GOLD_COLOR(176, 146, 82);

    // The game's menus use a serif face; Georgia is the closest common one on Windows.
    inline QFont serifFont(int pixelSize) {
        QFont font("Georgia");
        font.setStyleHint(QFont::Serif);
        font.setPixelSize(std::max(8, pixelSize));
        return font;
    }

    // Light text with the soft dark shadow the game puts under all menu text.
    inline void drawShadowText(QPainter& painter, const QRect& rect, int flags,
                               const QString& text, const QColor& color = TEXT_COLOR) {
        const QFontMetrics fm(painter.font());
        const QString elided = fm.elidedText(text, Qt::ElideRight, rect.width());
        const int offset = std::max(1, painter.font().pixelSize() / 14);
        painter.save();
        painter.setPen(QColor(0, 0, 0, 200));
        painter.drawText(rect.translated(offset, offset), flags, elided);
        painter.setPen(color);
        painter.drawText(rect, flags, elided);
        painter.restore();
    }

    // Thin gold rule under labels; fades out towards both ends.
    inline void drawGoldRule(QPainter& painter, int x1, int x2, int y, int alpha = 255) {
        if (x2 <= x1) return;
        QLinearGradient gradient(x1, 0, x2, 0);
        QColor gold = GOLD_COLOR;
        gold.setAlpha(alpha);
        QColor clear = gold;
        clear.setAlpha(0);
        gradient.setColorAt(0.0, clear);
        gradient.setColorAt(0.06, gold);
        gradient.setColorAt(0.85, gold);
        gradient.setColorAt(1.0, clear);
        painter.save();
        painter.setPen(QPen(QBrush(gradient), 1));
        painter.drawLine(x1, y, x2, y);
        painter.setPen(QPen(QColor(0, 0, 0, alpha / 2), 1));
        painter.drawLine(x1, y + 1, x2, y + 1);
        painter.restore();
    }

    // Ornamental double gold border with small square corners (item grid frame).
    inline void drawOrnateFrame(QPainter& painter, const QRect& rect, double scale) {
        painter.save();
        painter.setRenderHint(QPainter::Antialiasing, false);
        painter.setBrush(Qt::NoBrush);
        const int gap = std::max(3, int(6 * scale));
        painter.setPen(QPen(QColor(150, 124, 72, 200), 1));
        painter.drawRect(rect.adjusted(0, 0, -1, -1));
        painter.setPen(QPen(QColor(110, 92, 58, 160), 1));
        painter.drawRect(rect.adjusted(gap, gap, -gap - 1, -gap - 1));
        const int c = std::max(5, int(12 * scale));
        for (const QPoint& p : {rect.topLeft(), QPoint(rect.right() - c + 1, rect.top()),
                                QPoint(rect.left(), rect.bottom() - c + 1),
                                QPoint(rect.right() - c + 1, rect.bottom() - c + 1)}) {
            const QRect corner(p, QSize(c, c));
            painter.fillRect(corner, QColor(28, 26, 24));
            painter.setPen(QPen(QColor(176, 146, 82, 220), 1));
            painter.drawRect(corner.adjusted(0, 0, -1, -1));
            painter.drawRect(corner.adjusted(c / 4, c / 4, -c / 4 - 1, -c / 4 - 1));
        }
        painter.restore();
    }

    // Draws `pix` into `target` keeping its borders (in source pixels) unstretched, scaled by `scale`.
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

    // One of the game's translucent menu panels (tools/ds3_flags/gen_menu.py) stretched over
    // `rect`, keeping its ragged edges unstretched.
    inline void drawMenuPanel(QPainter& painter, const QRect& rect, const QString& resource) {
        static QHash<QString, QPixmap> cache;
        auto it = cache.find(resource);
        if (it == cache.end()) it = cache.insert(resource, QPixmap(resource));
        constexpr int EDGE = 24;
        drawNineSlice(painter, *it, rect, EDGE, EDGE, EDGE, EDGE, 1.0);
    }

    // Coiled sword in a bonfire: flames when lit, cold ash when not.
    inline void drawBonfire(QPainter& painter, const QRectF& rect, bool lit) {
        painter.save();
        painter.setRenderHint(QPainter::Antialiasing, true);
        const double w = rect.width();
        const double h = rect.height();
        const double cx = rect.center().x();
        // Ash mound
        painter.setPen(Qt::NoPen);
        painter.setBrush(lit ? QColor(70, 56, 46) : QColor(58, 55, 52));
        painter.drawEllipse(QRectF(cx - w * 0.42, rect.bottom() - h * 0.2, w * 0.84, h * 0.2));
        if (lit) {
            QPainterPath flame;
            const double base = rect.bottom() - h * 0.1;
            flame.moveTo(cx, rect.top() + h * 0.08);
            flame.cubicTo(cx + w * 0.18, rect.top() + h * 0.35, cx + w * 0.42, base - h * 0.25, cx + w * 0.3, base);
            flame.lineTo(cx - w * 0.3, base);
            flame.cubicTo(cx - w * 0.42, base - h * 0.25, cx - w * 0.18, rect.top() + h * 0.35, cx, rect.top() + h * 0.08);
            QRadialGradient gradient(QPointF(cx, base - h * 0.15), h * 0.75);
            gradient.setColorAt(0.0, QColor(255, 236, 160, 240));
            gradient.setColorAt(0.45, QColor(242, 140, 48, 220));
            gradient.setColorAt(1.0, QColor(170, 50, 20, 0));
            painter.setBrush(gradient);
            painter.drawPath(flame);
        }
        // Sword, thrust into the ash at a slight angle
        const QColor steel = lit ? QColor(60, 44, 34) : QColor(128, 122, 114);
        painter.setPen(QPen(steel, std::max(1.0, w * 0.09), Qt::SolidLine, Qt::RoundCap));
        const QPointF tip(cx + w * 0.02, rect.bottom() - h * 0.12);
        const QPointF hilt(cx - w * 0.06, rect.top() + h * 0.18);
        painter.drawLine(tip, hilt);
        painter.setPen(QPen(steel, std::max(1.0, w * 0.07), Qt::SolidLine, Qt::RoundCap));
        const QPointF guard = hilt + (tip - hilt) * 0.18;
        painter.drawLine(guard - QPointF(w * 0.16, w * 0.02), guard + QPointF(w * 0.16, -w * 0.02));
        painter.restore();
    }
}
