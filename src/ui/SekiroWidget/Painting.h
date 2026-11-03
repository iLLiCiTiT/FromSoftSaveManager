#pragma once
// Drawing helpers shared by the Sekiro views, which imitate the game's menus
// (ink brush strokes on a parchment board, see tools/sekiro_items/gen_menu.py).

#include <QFont>
#include <QFontMetrics>
#include <QHash>
#include <QPainter>
#include <QPixmap>
#include <QString>
#include <algorithm>
#include <cmath>

namespace fssm::ui::sekiro {
    inline const QColor TEXT_COLOR(240, 236, 226);
    inline const QColor DIM_TEXT_COLOR(196, 188, 170);
    inline const QColor SHADOW_COLOR(10, 9, 8, 200);
    inline const QColor ORANGE_COLOR(246, 160, 40);

    // The game's menu face is a Garamond-like serif. Garamond runs small, so the
    // requested size is bumped a little.
    inline QFont serifFont(int pixelSize) {
        QFont font;
        font.setFamilies({"Garamond", "Georgia", "Times New Roman"});
        font.setStyleHint(QFont::Serif);
        font.setPixelSize(std::max(8, int(std::lround(pixelSize * 1.12))));
        return font;
    }

    // Light text with a soft dark shadow, like the labels in the game.
    inline void drawShadowText(QPainter& painter, const QRect& rect, int flags, const QString& text,
                               const QColor& color = TEXT_COLOR) {
        QString shown = text;
        if (!(flags & Qt::TextWordWrap)) {
            shown = QFontMetrics(painter.font()).elidedText(text, Qt::ElideRight, rect.width());
        }
        painter.save();
        painter.setPen(SHADOW_COLOR);
        const int d = std::max(1, painter.font().pixelSize() / 14);
        painter.drawText(rect.translated(d, d), flags, shown);
        painter.drawText(rect.translated(0, d), flags, shown);
        painter.setPen(color);
        painter.drawText(rect, flags, shown);
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

    // Brush strokes are stretched to whatever box they fill; caches the scaled copies.
    class PixmapCache {
    public:
        QPixmap get(const QString& resource, const QSize& size) {
            if (size.isEmpty()) return {};
            const QString key = resource + QString("@%1x%2").arg(size.width()).arg(size.height());
            auto it = m_cache.find(key);
            if (it != m_cache.end()) return *it;
            const QPixmap source(resource);
            QPixmap result;
            if (!source.isNull()) result = source.scaled(size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
            m_cache.insert(key, result);
            return result;
        }
        // Keeps the aspect ratio, fitting inside `size`.
        QPixmap fit(const QString& resource, const QSize& size) {
            if (size.isEmpty()) return {};
            const QString key = resource + QString("#%1x%2").arg(size.width()).arg(size.height());
            auto it = m_cache.find(key);
            if (it != m_cache.end()) return *it;
            const QPixmap source(resource);
            QPixmap result;
            if (!source.isNull()) result = source.scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            m_cache.insert(key, result);
            return result;
        }
        void clear() { m_cache.clear(); }
    private:
        QHash<QString, QPixmap> m_cache;
    };

    inline QString formatPlayTime(uint32_t seconds) {
        return QString("%1:%2:%3").arg(seconds / 3600)
                                  .arg((seconds / 60) % 60, 2, 10, QChar('0'))
                                  .arg(seconds % 60, 2, 10, QChar('0'));
    }
}
