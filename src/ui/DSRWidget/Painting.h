#pragma once
// Drawing helpers shared by the DSR views, which imitate the game's menus: dark
// leather, thin bronze frames with curled corners, burnt orange selection and
// serif text with a soft shadow. Colors are sampled from in-game captures.

#include <QFont>
#include <QFontMetrics>
#include <QImage>
#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QRandomGenerator>
#include <algorithm>
#include <cmath>

namespace fssm::ui::dsr {
    inline const QColor TEXT_COLOR(234, 230, 220);
    inline const QColor DIM_TEXT_COLOR(150, 144, 132);
    // Area names and other secondary values.
    inline const QColor AREA_COLOR(196, 176, 138);
    // The Data List slot frames, toned down from an HDR capture.
    inline const QColor GOLD_COLOR(158, 128, 76);
    inline const QColor GOLD_LIGHT_COLOR(196, 166, 108);
    inline const QColor GOLD_DARK_COLOR(74, 56, 30);
    // Burnt orange leather of the selected slot or row.
    inline const QColor SELECTED_COLOR(150, 72, 30);
    // Thin bronze lines of the panel frames and rules.
    inline const QColor FRAME_COLOR(124, 100, 70);
    inline const QColor HOSTILE_COLOR(206, 92, 72);

    // The game's menus use a serif face; Georgia is the closest common one on Windows.
    // Its default numerals are old-style; the game's are lining.
    inline QFont serifFont(int pixelSize) {
        QFont font("Georgia");
        font.setStyleHint(QFont::Serif);
        font.setPixelSize(std::max(8, pixelSize));
        font.setFeature(QFont::Tag("lnum"), 1);
        return font;
    }

    // Light text with the soft dark shadow the game puts under all menu text.
    // Elided to the rect unless it wraps.
    inline void drawShadowText(QPainter& painter, const QRect& rect, int flags,
                               const QString& text, const QColor& color = TEXT_COLOR) {
        const QFontMetrics fm(painter.font());
        const QString shown = (flags & Qt::TextWordWrap) ? text : fm.elidedText(text, Qt::ElideRight, rect.width());
        const int offset = std::max(1, painter.font().pixelSize() / 14);
        painter.save();
        painter.setPen(QColor(0, 0, 0, 210));
        painter.drawText(rect.translated(offset, offset), flags, shown);
        painter.setPen(color);
        painter.drawText(rect, flags, shown);
        painter.restore();
    }

    // Tileable grain that gives flat fills the look of the game's worn leather.
    inline const QPixmap& grainTexture() {
        static const QPixmap texture = [] {
            QImage image(128, 128, QImage::Format_ARGB32_Premultiplied);
            QRandomGenerator rng(0xd5a);
            for (int y = 0; y < image.height(); ++y) {
                auto* line = reinterpret_cast<QRgb*>(image.scanLine(y));
                for (int x = 0; x < image.width(); ++x) {
                    const int alpha = int(rng.bounded(30));
                    const int v = rng.bounded(2) ? 255 : 0;
                    line[x] = qPremultiply(qRgba(v, v, v, alpha));
                }
            }
            return QPixmap::fromImage(image);
        }();
        return texture;
    }

    // Flat fill with the leather grain over it.
    inline void fillLeather(QPainter& painter, const QRect& rect, const QColor& color) {
        painter.fillRect(rect, color);
        painter.drawTiledPixmap(rect, grainTexture());
    }

    // Thin rule between stat groups: a bronze line over a dark one, fading out at both ends.
    inline void drawRule(QPainter& painter, int x1, int x2, int y, int alpha = 200) {
        if (x2 <= x1) return;
        QLinearGradient gradient(x1, 0, x2, 0);
        QColor line = FRAME_COLOR;
        line.setAlpha(alpha);
        QColor clear = line;
        clear.setAlpha(0);
        gradient.setColorAt(0.0, clear);
        gradient.setColorAt(0.04, line);
        gradient.setColorAt(0.96, line);
        gradient.setColorAt(1.0, clear);
        painter.save();
        painter.setRenderHint(QPainter::Antialiasing, false);
        painter.setPen(QPen(QColor(0, 0, 0, alpha * 3 / 4), 1));
        painter.drawLine(x1, y + 1, x2, y + 1);
        painter.setPen(QPen(QBrush(gradient), 1));
        painter.drawLine(x1, y, x2, y);
        painter.restore();
    }

    // Double rule with a small knot in the middle, between the inventory's item rows.
    inline void drawKnotRule(QPainter& painter, int x1, int x2, int y, double scale) {
        if (x2 <= x1) return;
        painter.save();
        painter.setRenderHint(QPainter::Antialiasing, true);
        const QColor line(FRAME_COLOR.red(), FRAME_COLOR.green(), FRAME_COLOR.blue(), 170);
        painter.setPen(QPen(line, 1));
        painter.drawLine(QPointF(x1, y - 1.5), QPointF(x2, y - 1.5));
        painter.drawLine(QPointF(x1, y + 1.5), QPointF(x2, y + 1.5));
        const double cx = (x1 + x2) / 2.0;
        const double r = std::max(2.0, 4.0 * scale);
        // Knot: two loops either side of a small bead.
        painter.setBrush(QColor(28, 26, 24));
        painter.setPen(QPen(FRAME_COLOR.lighter(115), 1));
        painter.drawEllipse(QPointF(cx - r * 1.6, y), r, r * 0.8);
        painter.drawEllipse(QPointF(cx + r * 1.6, y), r, r * 0.8);
        painter.setBrush(FRAME_COLOR);
        painter.drawEllipse(QPointF(cx, y), r * 0.6, r * 0.6);
        painter.restore();
    }

    // Panel frame of the Status and Inventory screens: a thin bronze double line with
    // curls at the corners. Draws nothing inside; the panels sit on the leather.
    inline void drawPanelFrame(QPainter& painter, const QRect& rect, double scale) {
        painter.save();
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setBrush(Qt::NoBrush);
        const double gap = std::max(3.0, 5.0 * scale);
        const QRectF outer = QRectF(rect).adjusted(0.5, 0.5, -0.5, -0.5);
        const QRectF inner = outer.adjusted(gap, gap, -gap, -gap);
        painter.setPen(QPen(QColor(0, 0, 0, 120), 3));
        painter.drawRect(outer);
        painter.setPen(QPen(FRAME_COLOR, 1.2));
        painter.drawRect(outer);
        painter.setPen(QPen(QColor(FRAME_COLOR.red(), FRAME_COLOR.green(), FRAME_COLOR.blue(), 150), 1));
        painter.drawRect(inner);

        // Corner curls: a short bar across the corner ending in a small spiral, mirrored.
        const double c = std::max(6.0, 14.0 * scale);
        painter.setPen(QPen(FRAME_COLOR.lighter(110), 1.2));
        const QPointF corners[4] = {outer.topLeft(), outer.topRight(), outer.bottomRight(), outer.bottomLeft()};
        const int sx[4] = {1, -1, -1, 1};
        const int sy[4] = {1, 1, -1, -1};
        for (int i = 0; i < 4; ++i) {
            const QPointF p = corners[i];
            const double dx = sx[i];
            const double dy = sy[i];
            // Bracket inside the corner.
            QPainterPath bracket;
            bracket.moveTo(p.x() + dx * c * 1.6, p.y() + dy * gap);
            bracket.cubicTo(p.x() + dx * c, p.y() + dy * gap, p.x() + dx * gap, p.y() + dy * c * 0.4,
                            p.x() + dx * gap, p.y() + dy * c * 1.6);
            painter.drawPath(bracket);
            // Curls just outside the corner along both edges.
            QPainterPath curl;
            curl.moveTo(p.x() + dx * c * 2.2, p.y());
            curl.cubicTo(p.x() + dx * c * 1.4, p.y() - dy * c * 0.5, p.x() + dx * c * 0.9, p.y() + dy * c * 0.1,
                         p.x() + dx * c * 1.3, p.y() + dy * c * 0.35);
            curl.moveTo(p.x(), p.y() + dy * c * 2.2);
            curl.cubicTo(p.x() - dx * c * 0.5, p.y() + dy * c * 1.4, p.x() + dx * c * 0.1, p.y() + dy * c * 0.9,
                         p.x() + dx * c * 0.35, p.y() + dy * c * 1.3);
            painter.drawPath(curl);
            painter.setBrush(FRAME_COLOR);
            painter.drawEllipse(QPointF(p.x() + dx * gap * 0.5, p.y() + dy * gap * 0.5), gap * 0.45, gap * 0.45);
            painter.setBrush(Qt::NoBrush);
        }
        painter.restore();
    }

    // Box with a thin bronze border, like the one around the name on the Status screen.
    inline void drawNameBox(QPainter& painter, const QRect& rect) {
        painter.save();
        painter.setRenderHint(QPainter::Antialiasing, false);
        painter.fillRect(rect, QColor(0, 0, 0, 60));
        painter.setPen(QPen(FRAME_COLOR, 1));
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(rect.adjusted(0, 0, -1, -1));
        painter.setPen(QPen(QColor(0, 0, 0, 140), 1));
        painter.drawRect(rect.adjusted(1, 1, -2, -2));
        painter.restore();
    }

    // Menu heading of the game's screens: the framed screen icon, the title on a rule and
    // a smaller subtitle on a second rule. The rules run to `ruleRight`.
    inline void drawMenuHeading(QPainter& painter, const QRect& rect, const QPixmap& icon,
                                const QString& title, const QString& subtitle, int ruleRight) {
        painter.save();
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
        const int iconSize = rect.height();
        if (!icon.isNull()) painter.drawPixmap(QRect(rect.left(), rect.top(), iconSize, iconSize), icon);
        const int textLeft = rect.left() + iconSize + std::max(4, iconSize / 8);
        const int titleH = rect.height() * 56 / 100;
        const QRect titleRect(textLeft, rect.top(), rect.right() - textLeft, titleH);
        const QRect subRect(textLeft + 2, rect.top() + titleH, rect.right() - textLeft - 2, rect.height() - titleH);
        painter.setFont(serifFont(titleH * 78 / 100));
        drawShadowText(painter, titleRect.adjusted(0, 0, 0, -1), Qt::AlignLeft | Qt::AlignVCenter, title);
        drawRule(painter, textLeft - 2, ruleRight, titleRect.bottom(), 190);
        if (!subtitle.isEmpty()) {
            painter.setFont(serifFont(subRect.height() * 70 / 100));
            drawShadowText(painter, subRect.adjusted(0, 0, 0, -1), Qt::AlignLeft | Qt::AlignVCenter, subtitle);
        }
        drawRule(painter, textLeft - 2, ruleRight, rect.bottom(), 150);
        painter.restore();
    }

    // Red wax seal the game puts on equipped items.
    inline void drawEquippedSeal(QPainter& painter, const QPointF& center, double radius) {
        painter.save();
        painter.setRenderHint(QPainter::Antialiasing, true);
        QRadialGradient gradient(center - QPointF(radius * 0.3, radius * 0.3), radius * 1.3);
        gradient.setColorAt(0.0, QColor(236, 96, 92));
        gradient.setColorAt(0.6, QColor(178, 32, 36));
        gradient.setColorAt(1.0, QColor(110, 14, 18));
        painter.setPen(QPen(QColor(90, 10, 12), 1));
        painter.setBrush(gradient);
        painter.drawEllipse(center, radius, radius);
        painter.setPen(QPen(QColor(255, 170, 160, 150), std::max(1.0, radius / 6)));
        painter.setBrush(Qt::NoBrush);
        painter.drawEllipse(center, radius * 0.55, radius * 0.55);
        painter.restore();
    }

    // Coiled sword in a bonfire: flames when lit, cold ash when not.
    inline void drawBonfire(QPainter& painter, const QRectF& rect, bool lit) {
        painter.save();
        painter.setRenderHint(QPainter::Antialiasing, true);
        const double w = rect.width();
        const double h = rect.height();
        const double cx = rect.center().x();
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

    // Greyscale copy keeping alpha, for the menu icons of the screens not shown,
    // as the game draws them.
    inline QPixmap greyscale(const QPixmap& pix) {
        QImage image = pix.toImage().convertToFormat(QImage::Format_ARGB32);
        for (int y = 0; y < image.height(); ++y) {
            auto* line = reinterpret_cast<QRgb*>(image.scanLine(y));
            for (int x = 0; x < image.width(); ++x) {
                const int v = qGray(line[x]);
                line[x] = qRgba(v, v, v, qAlpha(line[x]));
            }
        }
        return QPixmap::fromImage(image);
    }
}
