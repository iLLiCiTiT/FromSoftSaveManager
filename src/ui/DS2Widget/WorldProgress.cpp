#include "WorldProgress.h"

#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QWheelEvent>
#include <algorithm>
#include <climits>
#include <cstdlib>
#include <vector>

#include "Painting.h"
#include "../../parse/DS2/WorldProgress.h"

namespace fssm::ui::ds2 {
namespace {
    // Same window and borders as the Player Status screen.
    constexpr int WINDOW_W = 880;
    constexpr int WINDOW_H = 543;
    constexpr int FRAME_LEFT = 44;
    constexpr int FRAME_TOP = 66;
    constexpr int FRAME_RIGHT = 44;
    constexpr int FRAME_BOTTOM = 44;
    // Frame, fonts and rows keep one size (the look at the default window size);
    // resizing only changes how much fits.
    constexpr double FIXED_SCALE = 0.66;

    const QColor PANEL_COLOR(10, 10, 10, 120);
    const QColor HEADER_TOP(70, 84, 96, 230);
    const QColor HEADER_BOTTOM(24, 30, 36, 230);
    const QColor LINE_COLOR(96, 86, 68);
    const QColor AREA_COLOR(206, 186, 140);

    enum class RowKind {
        Area,       // area name with its count
        Bonfire,
        Boss,
    };

    struct Row {
        RowKind kind;
        QString text;
        QString value;
        bool done = false;
    };

    QString count(int done, int total) {
        return QString("%1 / %2").arg(done).arg(total);
    }

    QString text(std::string_view s) {
        return QString::fromUtf8(s.data(), int(s.size()));
    }

    // Rows grouped by area: `entries` is BONFIRES or BOSSES, `done` the matching flags.
    template <class Entries>
    std::vector<Row> groupedRows(const Entries& entries, const std::vector<bool>& done, RowKind kind) {
        using fssm::parse::ds2::AREAS;
        std::vector<Row> rows;
        for (size_t area = 0; area < AREAS.size(); ++area) {
            std::vector<Row> areaRows;
            int areaDone = 0;
            for (size_t i = 0; i < entries.size(); ++i) {
                if (entries[i].area != area) continue;
                const bool isDone = i < done.size() && done[i];
                areaRows.push_back({kind, text(entries[i].name), "", isDone});
                areaDone += isDone;
            }
            if (areaRows.empty()) continue;
            rows.push_back({RowKind::Area, text(AREAS[area]), count(areaDone, int(areaRows.size()))});
            rows.insert(rows.end(), areaRows.begin(), areaRows.end());
        }
        return rows;
    }

    // Flame over a small ash mound: bright when lit, a cold ember when not.
    void drawBonfireMark(QPainter& painter, const QRectF& rect, bool lit) {
        painter.save();
        painter.setRenderHint(QPainter::Antialiasing, true);
        const double w = rect.width();
        const double h = rect.height();
        const double cx = rect.center().x();
        const double base = rect.bottom() - h * 0.12;
        painter.setPen(Qt::NoPen);
        painter.setBrush(lit ? QColor(72, 58, 46) : QColor(60, 57, 54));
        painter.drawEllipse(QRectF(cx - w * 0.4, base - h * 0.08, w * 0.8, h * 0.18));
        QPainterPath flame;
        const double top = rect.top() + (lit ? h * 0.06 : h * 0.4);
        flame.moveTo(cx, top);
        flame.cubicTo(cx + w * 0.2, top + (base - top) * 0.35, cx + w * 0.36, base - h * 0.18, cx + w * 0.24, base);
        flame.lineTo(cx - w * 0.24, base);
        flame.cubicTo(cx - w * 0.36, base - h * 0.18, cx - w * 0.2, top + (base - top) * 0.35, cx, top);
        if (lit) {
            QRadialGradient gradient(QPointF(cx, base - h * 0.15), h * 0.7);
            gradient.setColorAt(0.0, QColor(255, 238, 170, 245));
            gradient.setColorAt(0.5, QColor(240, 136, 44, 230));
            gradient.setColorAt(1.0, QColor(160, 46, 18, 40));
            painter.setBrush(gradient);
        } else {
            painter.setBrush(QColor(92, 84, 76));
        }
        painter.drawPath(flame);
        painter.restore();
    }

    // Small diamond marking a boss: filled gold when defeated.
    void drawBossMark(QPainter& painter, const QRectF& rect, bool defeated) {
        painter.save();
        painter.setRenderHint(QPainter::Antialiasing, true);
        const QPointF c = rect.center();
        const double r = rect.width() / 2;
        const QPolygonF diamond({{c.x(), c.y() - r}, {c.x() + r, c.y()}, {c.x(), c.y() + r}, {c.x() - r, c.y()}});
        painter.setPen(QPen(defeated ? QColor(232, 200, 128) : QColor(110, 104, 96), std::max(1.0, r / 4)));
        painter.setBrush(defeated ? QBrush(QColor(196, 160, 88)) : QBrush(Qt::NoBrush));
        painter.drawPolygon(diamond);
        painter.restore();
    }

    void drawSectionHeader(QPainter& painter, const QRect& rect, const QString& title, const QString& value) {
        QLinearGradient bg(rect.topLeft(), rect.bottomLeft());
        bg.setColorAt(0.0, HEADER_TOP);
        bg.setColorAt(1.0, HEADER_BOTTOM);
        painter.setPen(QPen(LINE_COLOR, 1));
        painter.setBrush(bg);
        painter.drawRect(rect.adjusted(0, 0, -1, -1));
        const QRect inner = rect.adjusted(rect.height() / 3, 0, -rect.height() / 3, 0);
        drawOutlinedText(painter, inner, Qt::AlignLeft | Qt::AlignVCenter, title, TEXT_COLOR);
        drawOutlinedText(painter, inner, Qt::AlignRight | Qt::AlignVCenter, value, TEXT_COLOR);
    }
}

WorldProgressWidget::WorldProgressWidget(WorldPage page, QWidget* parent): QWidget(parent), m_page(page) {
    setAttribute(Qt::WA_TranslucentBackground, true);
    for (auto& scroll : m_scrolls) scroll.setContentDrag(true);
    m_window = QPixmap(":/ds2_images/ui_window");
}

QSize WorldProgressWidget::sizeHint() const {
    return {WINDOW_W, WINDOW_H};
}

void WorldProgressWidget::setCharacter(const fssm::parse::ds2::DS2CharacterInfo* charInfo) {
    const bool sameCharacter = charInfo != nullptr && m_hasCharacter && m_char.index == charInfo->index;
    if (!sameCharacter) for (auto& scroll : m_scrolls) scroll.reset();
    m_hasCharacter = charInfo != nullptr;
    if (charInfo != nullptr) m_char = *charInfo;
    update();
}

void WorldProgressWidget::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    painter.setClipRect(event->rect());
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRect r = rect();
    const double s = FIXED_SCALE;
    if (!m_window.isNull()) {
        drawNineSlice(painter, m_window, r, FRAME_LEFT, FRAME_TOP, FRAME_RIGHT, FRAME_BOTTOM, s);
    } else {
        painter.fillRect(r, QColor(32, 30, 28));
    }
    const QRect titleBar(r.left() + int(46 * s), r.top() + int(24 * s), r.width() - int(92 * s), int(36 * s));
    painter.setFont(serifFont(std::max(15, int(titleBar.height() * 0.68))));
    drawOutlinedText(painter, titleBar.adjusted(int(14 * s), 0, 0, 0), Qt::AlignLeft | Qt::AlignVCenter,
                     "World Progress", TEXT_COLOR);

    const QRect content = r.adjusted(int(FRAME_LEFT * s) + 4, int(FRAME_TOP * s) + 6,
                                     -(int(FRAME_RIGHT * s) + 4), -(int(FRAME_BOTTOM * s) + 2));
    for (auto& scroll : m_scrolls) scroll.clear();
    if (!m_hasCharacter) {
        painter.setFont(serifFont(int(20 * s)));
        painter.setPen(DIM_TEXT_COLOR);
        painter.drawText(content, Qt::AlignCenter, "No character selected");
        return;
    }
    if (m_char.bonfiresLit.empty() && m_char.bossesDefeated.empty()) {
        painter.setFont(serifFont(int(20 * s)));
        painter.setPen(DIM_TEXT_COLOR);
        painter.drawText(content, Qt::AlignCenter, "No world data in this save");
        return;
    }

    using fssm::parse::ds2::BONFIRES;
    using fssm::parse::ds2::BOSSES;
    const auto countTrue = [](const std::vector<bool>& v) { return int(std::count(v.begin(), v.end(), true)); };

    const int headerH = int(34 * s);
    const int gap = std::max(4, headerH / 4);
    const int lineH = std::max(14, int(headerH * 0.82));
    const int areaH = lineH + gap;
    const auto listHeight = [&](const std::vector<Row>& rows) {
        int h = 0;
        for (const Row& row : rows) h += row.kind == RowKind::Area ? areaH : lineH;
        return h;
    };

    // A framed panel with its header; its lists (one, or a long one split side by side)
    // scroll together. Without lists it shows message instead.
    struct Panel {
        QRect rect;
        QString title;
        QString total;
        std::vector<std::vector<Row>> lists;
        QString message;
    };
    std::vector<Panel> panels;
    if (m_page == WorldPage::Bonfires) {
        const std::vector<Row> rows = groupedRows(BONFIRES, m_char.bonfiresLit, RowKind::Bonfire);
        Panel panel {content, "Bonfires Lit", count(countTrue(m_char.bonfiresLit), int(BONFIRES.size())), {rows}};
        if (content.width() >= int(720 * s)) {
            // Split at the area start closest to the middle.
            const int half = listHeight(rows) / 2;
            size_t split = 0;
            int best = INT_MAX;
            int y = 0;
            for (size_t i = 0; i < rows.size(); ++i) {
                if (rows[i].kind == RowKind::Area && i > 0 && std::abs(y - half) < best) {
                    best = std::abs(y - half);
                    split = i;
                }
                y += rows[i].kind == RowKind::Area ? areaH : lineH;
            }
            if (split > 0) {
                panel.lists = {std::vector<Row>(rows.begin(), rows.begin() + split),
                               std::vector<Row>(rows.begin() + split, rows.end())};
            }
        }
        panels.push_back(panel);
    } else {
        const int colW = (content.width() - gap) / 2;
        panels.push_back({QRect(content.left(), content.top(), colW, content.height()), "Bosses Defeated",
                          count(countTrue(m_char.bossesDefeated), int(BOSSES.size())),
                          {groupedRows(BOSSES, m_char.bossesDefeated, RowKind::Boss)}});
        panels.push_back({QRect(content.right() - colW + 1, content.top(), colW, content.height()), "NPCs", "", {},
                          "No NPC data is available for Dark Souls II"});
    }

    for (int c = 0; c < int(panels.size()); ++c) {
        const Panel& panelInfo = panels[c];
        const QRect col = panelInfo.rect;
        painter.setPen(QPen(LINE_COLOR, 1));
        painter.setBrush(PANEL_COLOR);
        painter.drawRect(col.adjusted(0, 0, -1, -1));
        painter.setFont(serifFont(std::max(11, int(headerH * 0.62))));
        drawSectionHeader(painter, QRect(col.left(), col.top(), col.width(), headerH), panelInfo.title, panelInfo.total);

        const QRect list = col.adjusted(gap, headerH + gap / 2, -gap, -gap / 2);
        if (panelInfo.lists.empty()) {
            painter.setFont(serifFont(std::max(10, int(lineH * 0.62))));
            drawOutlinedText(painter, list, Qt::AlignCenter | Qt::TextWordWrap, panelInfo.message, DIM_TEXT_COLOR);
            continue;
        }
        PaintedScroll& scroll = m_scrolls[c];
        int contentH = 0;
        for (const auto& rows : panelInfo.lists) contentH = std::max(contentH, listHeight(rows));
        const int barW = std::max(3, gap / 2);
        scroll.setGeometry(list, QRect(list.right() - barW + 1, list.top(), barW, list.height()), contentH);
        scroll.setStep(3 * lineH);

        painter.save();
        painter.setClipRect(list, Qt::IntersectClip);
        const int iconSize = int(lineH * 0.72);
        const int scrollBarW = scroll.scrollable() ? barW + gap / 2 : 0;
        const int lists = int(panelInfo.lists.size());
        const int listGap = 3 * gap;
        const int listW = (list.width() - scrollBarW - listGap * (lists - 1)) / lists;
        for (int l = 0; l < lists; ++l) {
            int y = list.top() - scroll.value();
            for (const Row& row : panelInfo.lists[l]) {
                const int h = row.kind == RowKind::Area ? areaH : lineH;
                const QRect full(list.left() + l * (listW + listGap), y, listW, h);
                y += h;
                if (full.bottom() < list.top() || full.top() > list.bottom()) continue;
                if (row.kind == RowKind::Area) {
                    const QRect line(full.left(), full.bottom() - lineH + 1, full.width(), lineH);
                    painter.setFont(serifFont(std::max(10, int(lineH * 0.66))));
                    const QFontMetrics fm(painter.font());
                    const int valueW = fm.horizontalAdvance(row.value) + gap;
                    drawOutlinedText(painter, line.adjusted(2, 0, -valueW, 0), Qt::AlignLeft | Qt::AlignVCenter,
                                     row.text, AREA_COLOR);
                    drawOutlinedText(painter, line.adjusted(0, 0, -2, 0), Qt::AlignRight | Qt::AlignVCenter, row.value,
                                     AREA_COLOR);
                    painter.setPen(QPen(LINE_COLOR, 1));
                    painter.drawLine(line.left(), line.bottom(), line.right(), line.bottom());
                    continue;
                }
                const QRectF icon(full.left() + gap, full.center().y() - iconSize / 2.0, iconSize, iconSize);
                if (row.kind == RowKind::Bonfire) {
                    drawBonfireMark(painter, icon, row.done);
                } else {
                    drawBossMark(painter, icon.adjusted(iconSize * 0.2, iconSize * 0.2, -iconSize * 0.2, -iconSize * 0.2),
                                 row.done);
                }
                painter.setFont(serifFont(std::max(10, int(lineH * 0.62))));
                const int textLeft = int(icon.right()) + gap;
                drawOutlinedText(painter, QRect(textLeft, full.top(), full.right() - textLeft, full.height()),
                                 Qt::AlignLeft | Qt::AlignVCenter, row.text, row.done ? TEXT_COLOR : DIM_TEXT_COLOR);
            }
        }
        painter.restore();

        scroll.paint(painter, QColor(0, 0, 0, 120), QColor(150, 134, 100));
    }
}

void WorldProgressWidget::wheelEvent(QWheelEvent* event) {
    const int column = m_page == WorldPage::Bonfires || event->position().toPoint().x() < m_scrolls[1].viewport().left() ? 0 : 1;
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
