#include "WorldProgress.h"

#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QRadialGradient>
#include <QWheelEvent>

#include "../../parse/Sekiro/WorldProgress.h"

namespace fssm::ui::sekiro {
namespace {
    // Frame, fonts and rows keep one size (the look at the default window size);
    // resizing only changes how much fits.
    constexpr double FIXED_SCALE = 0.89;

    enum class RowKind {
        Area,       // travel menu area with its count
        Idol,
        Boss,
    };

    struct Row {
        RowKind kind;
        QString text;
        QString value;
        bool done = false;
    };

    QString label(std::string_view text) {
        return QString::fromUtf8(text.data(), int(text.size()));
    }

    QString count(int done, int total) {
        return QString("%1/%2").arg(done).arg(total);
    }

    bool isSet(const std::vector<bool>& values, size_t i) {
        return i < values.size() && values[i];
    }

    // Rows of one list grouped by area, and how many of its entries are done.
    template <typename Entries>
    std::vector<Row> groupedRows(const Entries& entries, const std::vector<bool>& done, RowKind kind, int& doneCount) {
        using fssm::parse::sekiro::AREAS;
        std::vector<Row> rows;
        doneCount = 0;
        for (size_t area = 0; area < AREAS.size(); ++area) {
            std::vector<Row> group;
            int areaDone = 0;
            for (size_t i = 0; i < entries.size(); ++i) {
                if (entries[i].area != area) continue;
                group.push_back({kind, label(entries[i].name), "", isSet(done, i)});
                areaDone += isSet(done, i);
            }
            if (group.empty()) continue;
            rows.push_back({RowKind::Area, label(AREAS[area]), count(areaDone, int(group.size()))});
            rows.insert(rows.end(), group.begin(), group.end());
            doneCount += areaDone;
        }
        return rows;
    }

    // Sculptor's Idol: a seated figure, its ember lit once the idol is found.
    void drawIdolMark(QPainter& painter, const QRectF& rect, bool found) {
        painter.save();
        const QPointF c = rect.center();
        const double r = rect.width() / 2;
        if (found) {
            QRadialGradient glow(c, r);
            glow.setColorAt(0.0, QColor(255, 210, 120, 230));
            glow.setColorAt(0.55, QColor(ORANGE_COLOR.red(), ORANGE_COLOR.green(), ORANGE_COLOR.blue(), 150));
            glow.setColorAt(1.0, QColor(ORANGE_COLOR.red(), ORANGE_COLOR.green(), ORANGE_COLOR.blue(), 0));
            painter.setPen(Qt::NoPen);
            painter.setBrush(glow);
            painter.drawEllipse(c, r, r);
        }
        const QColor ink = found ? QColor(40, 30, 22) : QColor(70, 66, 60, 200);
        painter.setPen(Qt::NoPen);
        painter.setBrush(ink);
        painter.drawEllipse(QPointF(c.x(), c.y() - r * 0.42), r * 0.2, r * 0.2);                     // head
        painter.drawRoundedRect(QRectF(c.x() - r * 0.36, c.y() - r * 0.2, r * 0.72, r * 0.6), r * 0.2, r * 0.2);
        painter.drawRoundedRect(QRectF(c.x() - r * 0.56, c.y() + r * 0.32, r * 1.12, r * 0.24), r * 0.1, r * 0.1);
        painter.restore();
    }

    // Red seal stamped on defeated bosses, an empty ink frame otherwise.
    void drawBossMark(QPainter& painter, const QRectF& rect, bool defeated) {
        painter.save();
        const QRectF seal = rect.adjusted(rect.width() * 0.12, rect.height() * 0.12,
                                          -rect.width() * 0.12, -rect.height() * 0.12);
        const double radius = seal.width() * 0.14;
        if (defeated) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(196, 36, 24, 235));
            painter.drawRoundedRect(seal, radius, radius);
            QFont font;
            font.setFamilies({"SimSun", "MingLiU", "Yu Gothic"});
            font.setPixelSize(std::max(6, int(seal.height() * 0.8)));
            painter.setFont(font);
            painter.setPen(QColor(255, 236, 220));
            painter.drawText(seal, Qt::AlignCenter, QString(QChar(0x8A0E)));     // 討, "struck down"
        } else {
            painter.setPen(QPen(QColor(70, 66, 60, 200), std::max(1.0, seal.width() / 10)));
            painter.setBrush(Qt::NoBrush);
            painter.drawRoundedRect(seal, radius, radius);
        }
        painter.restore();
    }
}

WorldProgressWidget::WorldProgressWidget(QWidget* parent): QWidget(parent) {
    setAttribute(Qt::WA_TranslucentBackground, true);
    for (auto& scroll : m_scrolls) scroll.setContentDrag(true);
}

void WorldProgressWidget::setProgress(const std::vector<bool>& idolsFound, const std::vector<bool>& bossesDefeated) {
    if (idolsFound != m_idolsFound || bossesDefeated != m_bossesDefeated) for (auto& scroll : m_scrolls) scroll.reset();
    m_idolsFound = idolsFound;
    m_bossesDefeated = bossesDefeated;
    update();
}

void WorldProgressWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    m_cache.clear();
}

void WorldProgressWidget::paintEvent(QPaintEvent* event) {
    using fssm::parse::sekiro::BOSSES;
    using fssm::parse::sekiro::IDOLS;

    QPainter painter(this);
    painter.setClipRect(event->rect());
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    const QRect r = rect();
    const double s = FIXED_SCALE;
    const auto px = [s](double v) { return int(std::round(v * s)); };

    int idolCount = 0;
    int bossCount = 0;
    const std::vector<Row> idols = groupedRows(IDOLS, m_idolsFound, RowKind::Idol, idolCount);
    const std::vector<Row> bosses = groupedRows(BOSSES, m_bossesDefeated, RowKind::Boss, bossCount);

    const int leftW = int(r.width() * 0.52);
    const int headerH = px(22);
    const int areaH = px(30);
    const int lineH = px(26);
    const int iconSize = px(20);

    // Left: idols on the board like the item list. Right: bosses in the detail panel.
    const QRect idolHeader(r.left() + px(14), r.top(), leftW - px(28), headerH);
    const QRect panel(r.left() + leftW + px(4), r.top() - px(4), r.width() - leftW - px(4), r.height() + px(4));
    painter.drawPixmap(panel.topLeft(), m_cache.get(":/sekiro_images/detail_panel", panel.size()));
    const QRect panelInner = panel.adjusted(px(14), px(12), -px(14), -px(14));
    const QRect bossTitle(panelInner.left(), panelInner.top(), panelInner.width(), px(32));
    std::array<QRect, 2> lists;
    lists[0] = QRect(r.left(), idolHeader.bottom() + 1 + px(8), leftW - px(16),
                       std::max(0, r.bottom() - idolHeader.bottom() - px(10)));
    lists[1] = QRect(panelInner.left(), bossTitle.bottom() + 1 + px(6), panelInner.width(),
                       std::max(0, panelInner.bottom() - bossTitle.bottom() - px(6)));

    painter.setOpacity(0.55);
    const QRect headerBar = idolHeader.adjusted(0, 0, -idolHeader.width() / 3, 0);
    painter.drawPixmap(headerBar.topLeft(), m_cache.get(":/sekiro_images/header_bar", headerBar.size()));
    painter.setOpacity(1.0);
    painter.setFont(serifFont(px(16)));
    drawShadowText(painter, idolHeader.adjusted(px(14), 0, 0, 0), Qt::AlignLeft | Qt::AlignVCenter,
                   QString("Sculptor's Idols  (%1)").arg(count(idolCount, int(IDOLS.size()))));

    painter.drawPixmap(bossTitle.topLeft(), m_cache.get(":/sekiro_images/title_bar", bossTitle.size()));
    painter.setFont(serifFont(px(18)));
    drawShadowText(painter, bossTitle.adjusted(px(24), 0, -px(24), 0), Qt::AlignCenter,
                   QString("Bosses  (%1)").arg(count(bossCount, int(BOSSES.size()))));

    const auto rowHeight = [&](const Row& row, bool first) {
        return row.kind == RowKind::Area ? (first ? 0 : px(6)) + areaH : lineH;
    };
    const auto drawRows = [&](const std::vector<Row>& rows, int column) {
        const QRect list = lists[column];
        PaintedScroll& scroll = m_scrolls[column];
        int contentH = 0;
        for (size_t i = 0; i < rows.size(); ++i) contentH += rowHeight(rows[i], i == 0);
        const int barW = std::max(2, px(3));
        scroll.setGeometry(list, QRect(list.right() - barW, list.top() + px(4), barW, std::max(0, list.height() - px(8))),
                           contentH);
        scroll.setStep(3 * lineH);
        scroll.setMinimumThumbHeight(px(18));

        painter.save();
        painter.setClipRect(list, Qt::IntersectClip);
        int y = list.top() - scroll.value();
        for (size_t i = 0; i < rows.size(); ++i) {
            const Row& row = rows[i];
            const int h = rowHeight(row, i == 0);
            const QRect full(list.left(), y, list.width(), h);
            y += h;
            if (full.bottom() < list.top() || full.top() > list.bottom()) continue;
            if (row.kind == RowKind::Area) {
                // Ink stroke like the inventory's item rows, area name and count on it.
                const QRect stroke(full.left() + px(10), full.bottom() - areaH + 1 + px(2), full.width() - px(10),
                                   areaH - px(4));
                painter.drawPixmap(stroke.topLeft(), m_cache.get(":/sekiro_images/item_row", stroke.size()));
                painter.setFont(serifFont(px(16)));
                const QFontMetrics fm(painter.font());
                const int valueW = fm.horizontalAdvance(row.value) + px(10);
                drawShadowText(painter, stroke.adjusted(px(16), 0, -valueW - px(22), 0), Qt::AlignLeft | Qt::AlignVCenter,
                               row.text);
                drawShadowText(painter, stroke.adjusted(0, 0, -px(26), 0), Qt::AlignRight | Qt::AlignVCenter, row.value);
                continue;
            }
            const QRectF icon(full.left() + px(22), full.center().y() - iconSize / 2.0, iconSize, iconSize);
            if (row.kind == RowKind::Idol) {
                drawIdolMark(painter, icon, row.done);
            } else {
                drawBossMark(painter, icon, row.done);
            }
            painter.setFont(serifFont(px(15)));
            const int textLeft = int(icon.right()) + px(10);
            // Dark ink on the board, light text in the panel, like the game.
            const QColor color = column == 0 ? (row.done ? QColor(26, 24, 20) : QColor(26, 24, 20, 120))
                                             : (row.done ? TEXT_COLOR : QColor(150, 144, 130));
            painter.setPen(color);
            const QRect text(textLeft, full.top(), full.right() - textLeft, full.height());
            if (column == 0) {
                painter.drawText(text, Qt::AlignLeft | Qt::AlignVCenter,
                                 QFontMetrics(painter.font()).elidedText(row.text, Qt::ElideRight, text.width()));
            } else {
                drawShadowText(painter, text, Qt::AlignLeft | Qt::AlignVCenter, row.text, color);
            }
        }
        painter.restore();

        scroll.paint(painter, QColor(30, 28, 24, 120), QColor(236, 230, 214), 1);
    };
    drawRows(idols, 0);
    drawRows(bosses, 1);
}

void WorldProgressWidget::wheelEvent(QWheelEvent* event) {
    const int column = event->position().toPoint().x() < m_scrolls[1].viewport().left() ? 0 : 1;
    m_scrolls[column].wheel(event, true);
}

void WorldProgressWidget::mousePressEvent(QMouseEvent* event) {
    for (auto& scroll : m_scrolls) {
        if (scroll.mousePress(event)) return;
    }
    QWidget::mousePressEvent(event);
}

void WorldProgressWidget::mouseMoveEvent(QMouseEvent* event) {
    bool used = false;
    for (auto& scroll : m_scrolls) used = scroll.mouseMove(event) || used;
    if (!used) QWidget::mouseMoveEvent(event);
}

void WorldProgressWidget::mouseReleaseEvent(QMouseEvent* event) {
    bool used = false;
    for (auto& scroll : m_scrolls) used = scroll.mouseRelease(event) || used;
    if (!used) QWidget::mouseReleaseEvent(event);
}

void WorldProgressWidget::leaveEvent(QEvent* event) {
    for (auto& scroll : m_scrolls) scroll.leave();
    QWidget::leaveEvent(event);
}
}
