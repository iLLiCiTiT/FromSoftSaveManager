#include "WorldProgress.h"

#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QWheelEvent>
#include <algorithm>
#include <climits>
#include <cmath>
#include <cstdlib>
#include <vector>

#include "Painting.h"
#include "../../parse/DSR/WorldProgress.h"

namespace fssm::ui::dsr {
namespace {
    // Same scale as the Status screen: rows and fonts keep one size; resizing only
    // changes how much fits.
    constexpr double FIXED_SCALE = 0.5;
    const QColor UNDONE_MARK_COLOR(120, 116, 110);

    enum class RowKind {
        Heading,    // "Bonfires 30/ 44"
        Area,       // area with its lit count
        Bonfire,
        Boss,
        Npc,
        NpcStep,    // a questline step or event of the NPC above, checked when done
    };

    using fssm::parse::dsr::NpcState;

    struct Row {
        RowKind kind;
        QString text;
        QString value;
        bool done = false;
        NpcState npc = NpcState::Unknown;
        QString detail;     // second, smaller line: an NPC's questline stage
    };

    QString count(int done, int total) {
        return QString("%1/%2").arg(done).arg(total);
    }

    QString fromView(std::string_view s) {
        return QString::fromUtf8(s.data(), int(s.size()));
    }

    std::vector<Row> bonfireRows(const fssm::parse::dsr::DSRCharacterInfo& c) {
        using fssm::parse::dsr::AREAS;
        using fssm::parse::dsr::BONFIRES;
        const auto isLit = [&c](size_t i) { return i < c.bonfiresLit.size() && c.bonfiresLit[i]; };
        int total = 0;
        for (size_t i = 0; i < BONFIRES.size(); ++i) total += isLit(i);
        std::vector<Row> rows = {{RowKind::Heading, "Bonfires", count(total, int(BONFIRES.size()))}};
        for (size_t area = 0; area < AREAS.size(); ++area) {
            std::vector<Row> entries;
            int lit = 0;
            for (size_t i = 0; i < BONFIRES.size(); ++i) {
                if (BONFIRES[i].area != area) continue;
                entries.push_back({RowKind::Bonfire, fromView(BONFIRES[i].name), "", isLit(i)});
                lit += isLit(i);
            }
            if (entries.empty()) continue;
            rows.push_back({RowKind::Area, fromView(AREAS[area]), count(lit, int(entries.size()))});
            rows.insert(rows.end(), entries.begin(), entries.end());
        }
        return rows;
    }

    std::vector<Row> bossRows(const fssm::parse::dsr::DSRCharacterInfo& c) {
        using fssm::parse::dsr::BOSSES;
        std::vector<Row> rows;
        int defeated = 0;
        for (size_t i = 0; i < BOSSES.size(); ++i) {
            const bool done = i < c.bossesDefeated.size() && c.bossesDefeated[i];
            rows.push_back({RowKind::Boss, fromView(BOSSES[i].name), "", done});
            defeated += done;
        }
        rows.insert(rows.begin(), Row{RowKind::Heading, "Bosses", count(defeated, int(BOSSES.size()))});
        return rows;
    }

    // NPCs by their event flags; the state is shown right aligned (nothing while the NPC's
    // map hasn't loaded yet), the questline stage below the name and the NPC's progress
    // steps as a checklist under it.
    std::vector<Row> npcRows(const fssm::parse::dsr::DSRCharacterInfo& c) {
        using fssm::parse::dsr::NPCS;
        using fssm::parse::dsr::NPC_STEPS;
        std::vector<Row> rows = {{RowKind::Heading, "NPCs", ""}};
        for (size_t i = 0; i < NPCS.size(); ++i) {
            const NpcState state = i < c.npcStates.size() ? c.npcStates[i] : NpcState::Unknown;
            QString value;
            switch (state) {
                case NpcState::Alive: value = "Alive"; break;
                case NpcState::Hostile: value = "Hostile"; break;
                case NpcState::Dead: value = "Dead"; break;
                case NpcState::Unknown: break;
            }
            const std::string_view stage = i < c.npcStages.size() ? c.npcStages[i] : std::string_view();
            rows.push_back({RowKind::Npc, fromView(NPCS[i].name), value,
                            state == NpcState::Alive || state == NpcState::Hostile, state, fromView(stage)});
            for (size_t s = 0; s < NPC_STEPS.size(); ++s) {
                if (NPC_STEPS[s].npcFlag != NPCS[i].firstFlag) continue;
                const bool done = s < c.npcStepsDone.size() && c.npcStepsDone[s];
                rows.push_back({RowKind::NpcStep, fromView(NPC_STEPS[s].text), "", done});
            }
        }
        return rows;
    }

    // Small diamond marking a boss: filled when defeated.
    void drawBossMark(QPainter& painter, const QRectF& rect, bool defeated) {
        painter.save();
        const QPointF c = rect.center();
        const double r = rect.width() / 2;
        const QPolygonF diamond({{c.x(), c.y() - r}, {c.x() + r, c.y()}, {c.x(), c.y() + r}, {c.x() - r, c.y()}});
        painter.setPen(QPen(defeated ? AREA_COLOR : UNDONE_MARK_COLOR, std::max(1.0, r / 4)));
        painter.setBrush(defeated ? QBrush(AREA_COLOR) : QBrush(Qt::NoBrush));
        painter.drawPolygon(diamond);
        painter.restore();
    }

    // Small square marking an NPC's progress step: filled and ticked when done.
    void drawStepMark(QPainter& painter, const QRectF& rect, bool done) {
        painter.save();
        const double w = std::max(1.0, rect.width() / 8);
        painter.setPen(QPen(done ? AREA_COLOR : UNDONE_MARK_COLOR, w));
        painter.setBrush(done ? QBrush(AREA_COLOR.darker(160)) : QBrush(Qt::NoBrush));
        painter.drawRect(rect);
        if (done) {
            QPen tick(TEXT_COLOR, w * 1.4);
            tick.setCapStyle(Qt::RoundCap);
            tick.setJoinStyle(Qt::RoundJoin);
            painter.setPen(tick);
            painter.setBrush(Qt::NoBrush);
            const double x = rect.left();
            const double y = rect.top();
            const double s = rect.width();
            painter.drawPolyline(QPolygonF({{x + s * 0.22, y + s * 0.52}, {x + s * 0.42, y + s * 0.72}, {x + s * 0.78, y + s * 0.3}}));
        }
        painter.restore();
    }

    // Small circle marking an NPC: filled while alive (red when hostile), crossed out when dead.
    void drawNpcMark(QPainter& painter, const QRectF& rect, NpcState state) {
        painter.save();
        const double w = std::max(1.0, rect.width() / 8);
        const QColor edge = state == NpcState::Hostile ? HOSTILE_COLOR
            : state == NpcState::Alive ? AREA_COLOR : UNDONE_MARK_COLOR;
        painter.setPen(QPen(edge, w));
        painter.setBrush(state == NpcState::Alive ? QBrush(AREA_COLOR)
                         : state == NpcState::Hostile ? QBrush(HOSTILE_COLOR.darker(130)) : QBrush(Qt::NoBrush));
        painter.drawEllipse(rect);
        if (state == NpcState::Dead) {
            const QRectF x = rect.adjusted(rect.width() * 0.25, rect.height() * 0.25, -rect.width() * 0.25, -rect.height() * 0.25);
            painter.drawLine(x.topLeft(), x.bottomRight());
            painter.drawLine(x.topRight(), x.bottomLeft());
        }
        painter.restore();
    }
}

WorldProgressWidget::WorldProgressWidget(WorldPage page, QWidget* parent): QWidget(parent), m_page(page) {
    setAttribute(Qt::WA_TranslucentBackground, true);
    m_scroll.setContentDrag(true);
    m_npcScroll.setContentDrag(true);
}

void WorldProgressWidget::setCharacter(const fssm::parse::dsr::DSRCharacterInfo* charInfo) {
    const bool sameCharacter = charInfo != nullptr && m_character.has_value() && m_character->index == charInfo->index;
    if (!sameCharacter) {
        m_scroll.reset();
        m_npcScroll.reset();
    }
    if (charInfo != nullptr) {
        m_character = *charInfo;
    } else {
        m_character.reset();
    }
    update();
}

void WorldProgressWidget::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    painter.setClipRect(event->rect());
    painter.setRenderHint(QPainter::Antialiasing, true);

    const double s = FIXED_SCALE;
    const auto px = [s](double v) { return int(std::round(v * s)); };
    if (!m_character.has_value()) {
        m_scroll.clear();
        m_npcScroll.clear();
        painter.setFont(serifFont(px(36)));
        drawShadowText(painter, rect(), Qt::AlignCenter, "No character selected", DIM_TEXT_COLOR);
        return;
    }
    const auto& c = *m_character;

    const QFont headingFont = serifFont(px(34));
    const QFont textFont = serifFont(px(28));
    const QFont detailFont = serifFont(px(23));
    const int lineH = px(46);
    const int detailH = px(32);
    const int stepH = px(36);
    const int areaH = px(50);
    const int areaGap = px(16);
    const int headingH = px(58);
    const int iconSize = px(34);
    const int scrollBarW = px(20);
    const int pad = px(30);

    const auto rowHeight = [&](const Row& row, bool first) {
        switch (row.kind) {
            case RowKind::Heading: return (first ? 0 : px(32)) + headingH;
            case RowKind::Area: return areaGap + areaH;
            case RowKind::NpcStep: return stepH;
            default: return row.detail.isEmpty() ? lineH : lineH + detailH;
        }
    };
    const auto columnHeight = [&](const std::vector<Row>& rows) {
        int h = 0;
        for (size_t i = 0; i < rows.size(); ++i) h += rowHeight(rows[i], i == 0);
        return h;
    };

    // Draws rows into area, the first row at top (already scrolled), clipped to clip.
    const auto drawColumn = [&](const std::vector<Row>& rows, const QRect& area, int top, const QRect& clip) {
        painter.save();
        painter.setClipRect(clip.adjusted(0, 0, 0, 1), Qt::IntersectClip);
        const int x = area.left();
        const int columnWidth = area.width();
        int y = top;
        for (size_t i = 0; i < rows.size(); ++i) {
            const Row& row = rows[i];
            const int h = rowHeight(row, i == 0);
            const QRect full(x, y, columnWidth, h);
            y += h;
            if (full.bottom() < clip.top() || full.top() > clip.bottom()) continue;
            switch (row.kind) {
                case RowKind::Heading: {
                    const QRect line(x, full.bottom() - headingH + 1, columnWidth, headingH);
                    painter.setFont(headingFont);
                    drawShadowText(painter, line.adjusted(px(4), 0, 0, -px(4)), Qt::AlignLeft | Qt::AlignVCenter, row.text);
                    drawShadowText(painter, line.adjusted(0, 0, -px(4), -px(4)), Qt::AlignRight | Qt::AlignVCenter, row.value);
                    drawRule(painter, line.left(), line.right(), line.bottom() - 1);
                    break;
                }
                case RowKind::Area: {
                    const QRect line(x, full.bottom() - areaH + 1, columnWidth, areaH);
                    painter.setFont(textFont);
                    const QFontMetrics fm(painter.font());
                    const int valueW = fm.horizontalAdvance(row.value) + px(20);
                    drawShadowText(painter, line.adjusted(px(4), 0, -valueW, -px(2)), Qt::AlignLeft | Qt::AlignVCenter,
                                   row.text, AREA_COLOR);
                    drawShadowText(painter, line.adjusted(0, 0, -px(4), -px(2)), Qt::AlignRight | Qt::AlignVCenter,
                                   row.value, AREA_COLOR);
                    drawRule(painter, line.left(), line.right(), line.bottom() - 1, 140);
                    break;
                }
                case RowKind::NpcStep: {
                    // Under the NPC's name, a little below the stage line.
                    const int markSize = px(20);
                    const int left = x + px(20) + iconSize + px(16);
                    const QRectF mark(left, full.center().y() - markSize / 2.0, markSize, markSize);
                    drawStepMark(painter, mark, row.done);
                    const QRect textRect(int(mark.right()) + px(12), full.top(), full.right() - int(mark.right()) - px(20), full.height());
                    painter.setFont(detailFont);
                    drawShadowText(painter, textRect, Qt::AlignLeft | Qt::AlignVCenter, row.text,
                                   row.done ? TEXT_COLOR : DIM_TEXT_COLOR);
                    break;
                }
                case RowKind::Bonfire:
                case RowKind::Boss:
                case RowKind::Npc: {
                    const QRect line(full.left(), full.top(), full.width(), lineH);
                    const QRectF icon(x + px(20), line.center().y() - iconSize / 2.0, iconSize, iconSize);
                    const QRectF smallIcon = icon.adjusted(iconSize * 0.2, iconSize * 0.2, -iconSize * 0.2, -iconSize * 0.2);
                    if (row.kind == RowKind::Bonfire) {
                        drawBonfire(painter, icon, row.done);
                    } else if (row.kind == RowKind::Boss) {
                        drawBossMark(painter, smallIcon, row.done);
                    } else {
                        drawNpcMark(painter, smallIcon, row.npc);
                    }
                    painter.setFont(textFont);
                    const QFontMetrics fm(painter.font());
                    const int textLeft = int(icon.right()) + px(16);
                    int valueW = 0;
                    if (!row.value.isEmpty()) {
                        valueW = fm.horizontalAdvance(row.value) + px(20);
                        const QColor valueColor = row.npc == NpcState::Hostile ? HOSTILE_COLOR
                            : row.done ? AREA_COLOR : DIM_TEXT_COLOR;
                        drawShadowText(painter, line.adjusted(0, 0, -px(4), 0), Qt::AlignRight | Qt::AlignVCenter,
                                       row.value, valueColor);
                    }
                    drawShadowText(painter, QRect(textLeft, line.top(), line.right() - textLeft - valueW, line.height()),
                                   Qt::AlignLeft | Qt::AlignVCenter, row.text, row.done ? TEXT_COLOR : DIM_TEXT_COLOR);
                    if (!row.detail.isEmpty()) {
                        const QRect detailRect(textLeft, line.bottom() - px(6), full.right() - textLeft - px(4), detailH);
                        painter.setFont(detailFont);
                        drawShadowText(painter, detailRect, Qt::AlignLeft | Qt::AlignTop, row.detail,
                                       row.done ? AREA_COLOR : DIM_TEXT_COLOR);
                    }
                    break;
                }
            }
        }
        painter.restore();
    };
    // Thin scroll bar along a panel's inner right edge.
    const int barW = std::max(2, px(6));
    const auto barTrack = [&](const QRect& column) {
        return QRect(column.right() - barW + 1, column.top(), barW, column.height());
    };
    const QColor trackColor(0, 0, 0, 120);
    const int gap = px(16);
    const QRect page = rect();
    const bool twoColumns = page.width() >= px(960);

    if (m_page == WorldPage::Bonfires) {
        // One framed panel; the heading across it stays put and the areas scroll below
        // it, in two balanced columns when it fits.
        m_npcScroll.clear();
        drawPanelFrame(painter, page, s * 1.4);
        const QRect body = page.adjusted(pad, pad, -pad + px(10), -pad);
        std::vector<Row> rows = bonfireRows(c);
        const std::vector<Row> heading(rows.begin(), rows.begin() + 1);
        rows.erase(rows.begin());
        const int contentW = body.width() - scrollBarW;
        const int columnGap = px(48);
        const int columnWidth = twoColumns ? (contentW - columnGap) / 2 : contentW;
        size_t split = rows.size();
        if (twoColumns) {
            // The area start closest to the middle of the list.
            const int half = columnHeight(rows) / 2;
            int y = 0;
            int best = INT_MAX;
            for (size_t i = 0; i < rows.size(); ++i) {
                if (rows[i].kind == RowKind::Area && i > 0 && std::abs(y - half) < best) {
                    best = std::abs(y - half);
                    split = i;
                }
                y += rowHeight(rows[i], false);
            }
        }
        const std::vector<Row> left(rows.begin(), rows.begin() + split);
        const std::vector<Row> right(rows.begin() + split, rows.end());
        const QRect list = body.adjusted(0, rowHeight(heading.front(), true), 0, 0);
        m_scroll.setGeometry(list, barTrack(list), std::max(columnHeight(left), columnHeight(right)));
        m_scroll.setStep(3 * lineH);
        const int top = list.top() - m_scroll.value();
        drawColumn(heading, QRect(body.left(), 0, contentW, 0), body.top(), body);
        drawColumn(left, QRect(body.left(), 0, columnWidth, 0), top, list);
        if (!right.empty()) drawColumn(right, QRect(body.left() + columnWidth + columnGap, 0, columnWidth, 0), top, list);
        m_scroll.paint(painter, trackColor, GOLD_COLOR);
        return;
    }

    // Bosses and NPCs in framed panels side by side, or stacked when narrow; each
    // panel scrolls on its own.
    QRect bossPanel;
    QRect npcPanel;
    if (twoColumns) {
        const int w = (page.width() - gap) / 2;
        bossPanel = QRect(page.left(), page.top(), w, page.height());
        npcPanel = QRect(page.right() - w + 1, page.top(), w, page.height());
    } else {
        const int h = (page.height() - gap) / 2;
        bossPanel = QRect(page.left(), page.top(), page.width(), h);
        npcPanel = QRect(page.left(), page.bottom() - h + 1, page.width(), h);
    }
    // Each panel's heading stays put; its rows scroll below it.
    const auto drawProgressPanel = [&](const std::vector<Row>& rows, const QRect& panel, PaintedScroll& scroll) {
        drawPanelFrame(painter, panel, s * 1.4);
        const QRect area = panel.adjusted(pad, pad, -pad + px(10), -pad);
        const std::vector<Row> heading(rows.begin(), rows.begin() + 1);
        const std::vector<Row> entries(rows.begin() + 1, rows.end());
        const QRect list = area.adjusted(0, rowHeight(heading.front(), true) + px(8), 0, 0);
        scroll.setGeometry(list, barTrack(list), columnHeight(entries));
        scroll.setStep(3 * lineH);
        drawColumn(heading, area.adjusted(0, 0, -scrollBarW, 0), area.top(), area);
        drawColumn(entries, list.adjusted(0, 0, -scrollBarW, 0), list.top() - scroll.value(), list);
        scroll.paint(painter, trackColor, GOLD_COLOR);
    };
    drawProgressPanel(bossRows(c), bossPanel, m_scroll);
    drawProgressPanel(npcRows(c), npcPanel, m_npcScroll);
}

void WorldProgressWidget::wheelEvent(QWheelEvent* event) {
    if (!m_character.has_value()) return;
    // The bonfires page scrolls from anywhere, the progress columns under the pointer.
    if (m_page == WorldPage::Bonfires) {
        m_scroll.wheel(event, true);
    } else if (!m_scroll.wheel(event)) {
        m_npcScroll.wheel(event);
    }
}

void WorldProgressWidget::mousePressEvent(QMouseEvent* event) {
    if (!m_scroll.mousePress(event) && !m_npcScroll.mousePress(event)) QWidget::mousePressEvent(event);
}

void WorldProgressWidget::mouseMoveEvent(QMouseEvent* event) {
    if (!m_scroll.mouseMove(event) && !m_npcScroll.mouseMove(event)) QWidget::mouseMoveEvent(event);
}

void WorldProgressWidget::mouseReleaseEvent(QMouseEvent* event) {
    if (!m_scroll.mouseRelease(event) && !m_npcScroll.mouseRelease(event)) QWidget::mouseReleaseEvent(event);
}

void WorldProgressWidget::leaveEvent(QEvent* event) {
    m_scroll.leave();
    m_npcScroll.leave();
    QWidget::leaveEvent(event);
}
}
