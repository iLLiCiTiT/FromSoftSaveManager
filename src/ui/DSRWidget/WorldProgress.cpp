#include "WorldProgress.h"

#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QWheelEvent>
#include <algorithm>
#include <climits>
#include <cstdlib>
#include <vector>

#include "../../parse/DSR/WorldProgress.h"

namespace fssm::ui::dsr {
namespace {
    const QColor TEXT_COLOR(255, 255, 255);
    const QColor DIM_TEXT_COLOR(255, 255, 255, 90);
    const QColor AREA_COLOR(169, 155, 130);
    const QColor RULE_COLOR(169, 155, 130, 110);
    const QColor UNDONE_MARK_COLOR(120, 116, 110);
    const QColor HOSTILE_COLOR(206, 92, 72);

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
        return QString("%1/ %2").arg(done).arg(total);
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

    // Flame when lit, a grey outline when not.
    void drawBonfireMark(QPainter& painter, const QRectF& rect, bool lit) {
        painter.save();
        const double w = rect.width();
        const double h = rect.height();
        const double cx = rect.center().x();
        const double top = rect.top() + h * 0.08;
        const double base = rect.bottom() - h * 0.08;
        QPainterPath flame;
        flame.moveTo(cx, top);
        flame.cubicTo(cx + w * 0.2, top + h * 0.3, cx + w * 0.42, base - h * 0.25, cx, base);
        flame.cubicTo(cx - w * 0.42, base - h * 0.25, cx - w * 0.2, top + h * 0.3, cx, top);
        if (lit) {
            QRadialGradient gradient(QPointF(cx, base - h * 0.2), h * 0.8);
            gradient.setColorAt(0.0, QColor(255, 236, 160));
            gradient.setColorAt(0.5, QColor(242, 140, 48));
            gradient.setColorAt(1.0, QColor(170, 50, 20));
            painter.setPen(Qt::NoPen);
            painter.setBrush(gradient);
        } else {
            painter.setPen(QPen(UNDONE_MARK_COLOR, std::max(1.0, w / 12)));
            painter.setBrush(Qt::NoBrush);
        }
        painter.drawPath(flame);
        painter.restore();
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

    const QRect body = rect().adjusted(4, 4, -4, -4);
    if (!m_character.has_value()) {
        m_scroll.clear();
        m_npcScroll.clear();
        painter.setPen(DIM_TEXT_COLOR);
        painter.drawText(body, Qt::AlignCenter, "No character selected");
        return;
    }
    const auto& c = *m_character;

    const QFont textFont = font();
    QFont headingFont = font();
    headingFont.setPointSizeF(textFont.pointSizeF() * 1.25);
    headingFont.setBold(true);
    const QFontMetrics fm(textFont);
    QFont detailFont = font();
    detailFont.setPointSizeF(textFont.pointSizeF() * 0.85);
    const QFontMetrics detailFm(detailFont);
    const int lineH = fm.height() + 6;
    const int detailH = detailFm.height() + 2;
    const int stepH = detailFm.height() + 3;
    const int areaH = lineH + 2;
    const int areaGap = 8;
    const int headingH = QFontMetrics(headingFont).height() + 10;
    const int iconSize = fm.height() - 2;
    const int scrollBarW = 10;

    const auto rowHeight = [&](const Row& row, bool first) {
        switch (row.kind) {
            case RowKind::Heading: return (first ? 0 : 16) + headingH;
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

    const auto drawRule = [&](int left, int right, int y) {
        painter.setPen(QPen(RULE_COLOR, 1));
        painter.drawLine(left, y, right, y);
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
                    painter.setPen(TEXT_COLOR);
                    painter.drawText(line.adjusted(4, 0, 0, 0), Qt::AlignLeft | Qt::AlignVCenter, row.text);
                    painter.drawText(line.adjusted(0, 0, -4, 0), Qt::AlignRight | Qt::AlignVCenter, row.value);
                    drawRule(line.left(), line.right(), line.bottom());
                    break;
                }
                case RowKind::Area: {
                    const QRect line(x, full.bottom() - areaH + 1, columnWidth, areaH);
                    painter.setFont(textFont);
                    painter.setPen(AREA_COLOR);
                    const int valueW = fm.horizontalAdvance(row.value) + 10;
                    painter.drawText(line.adjusted(4, 0, -valueW, 0), Qt::AlignLeft | Qt::AlignVCenter,
                                     fm.elidedText(row.text, Qt::ElideRight, line.width() - valueW - 4));
                    painter.drawText(line.adjusted(0, 0, -4, 0), Qt::AlignRight | Qt::AlignVCenter, row.value);
                    drawRule(line.left(), line.right(), line.bottom() - 1);
                    break;
                }
                case RowKind::NpcStep: {
                    // Under the NPC's name, a little below the stage line.
                    const int markSize = std::max(6, detailFm.height() - 6);
                    const int left = x + 10 + iconSize + 8;
                    const QRectF mark(left, full.center().y() - markSize / 2.0, markSize, markSize);
                    drawStepMark(painter, mark, row.done);
                    const QRect textRect(int(mark.right()) + 6, full.top(), full.right() - int(mark.right()) - 10, full.height());
                    painter.setFont(detailFont);
                    painter.setPen(row.done ? TEXT_COLOR : DIM_TEXT_COLOR);
                    painter.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter,
                                     detailFm.elidedText(row.text, Qt::ElideRight, textRect.width()));
                    break;
                }
                case RowKind::Bonfire:
                case RowKind::Boss:
                case RowKind::Npc: {
                    const QRect line(full.left(), full.top(), full.width(), lineH);
                    const QRectF icon(x + 10, line.center().y() - iconSize / 2.0, iconSize, iconSize);
                    const QRectF smallIcon = icon.adjusted(iconSize * 0.2, iconSize * 0.2, -iconSize * 0.2, -iconSize * 0.2);
                    if (row.kind == RowKind::Bonfire) {
                        drawBonfireMark(painter, icon, row.done);
                    } else if (row.kind == RowKind::Boss) {
                        drawBossMark(painter, smallIcon, row.done);
                    } else {
                        drawNpcMark(painter, smallIcon, row.npc);
                    }
                    painter.setFont(textFont);
                    const int textLeft = int(icon.right()) + 8;
                    int valueW = 0;
                    if (!row.value.isEmpty()) {
                        valueW = fm.horizontalAdvance(row.value) + 10;
                        painter.setPen(row.npc == NpcState::Hostile ? HOSTILE_COLOR : row.done ? AREA_COLOR : DIM_TEXT_COLOR);
                        painter.drawText(line.adjusted(0, 0, -4, 0), Qt::AlignRight | Qt::AlignVCenter, row.value);
                    }
                    painter.setPen(row.done ? TEXT_COLOR : DIM_TEXT_COLOR);
                    const QRect textRect(textLeft, line.top(), line.right() - textLeft - valueW, line.height());
                    painter.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter,
                                     fm.elidedText(row.text, Qt::ElideRight, textRect.width()));
                    if (!row.detail.isEmpty()) {
                        const QRect detailRect(textLeft, line.bottom() - 3, full.right() - textLeft - 4, detailH);
                        painter.setFont(detailFont);
                        painter.setPen(row.done ? AREA_COLOR : DIM_TEXT_COLOR);
                        painter.drawText(detailRect, Qt::AlignLeft | Qt::AlignTop,
                                         detailFm.elidedText(row.detail, Qt::ElideRight, detailRect.width()));
                    }
                    break;
                }
            }
        }
        painter.restore();
    };
    const auto barTrack = [](const QRect& column) {
        return QRect(column.right() - 2, column.top(), 3, column.height());
    };
    const QColor trackColor(0, 0, 0, 120);
    const int gap = 24;
    const bool twoColumns = body.width() >= 480;

    if (m_page == WorldPage::Bonfires) {
        // The heading across the page stays put; the areas scroll below it, in two balanced
        // columns when it fits.
        m_npcScroll.clear();
        std::vector<Row> rows = bonfireRows(c);
        const std::vector<Row> heading(rows.begin(), rows.begin() + 1);
        rows.erase(rows.begin());
        const int contentW = body.width() - scrollBarW;
        const int columnWidth = twoColumns ? (contentW - gap) / 2 : contentW;
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
        if (!right.empty()) drawColumn(right, QRect(body.left() + columnWidth + gap, 0, columnWidth, 0), top, list);
        m_scroll.paint(painter, trackColor, AREA_COLOR);
        return;
    }

    // Bosses and NPCs side by side, or stacked when narrow; each column has its own scroll bar.
    QRect bossArea;
    QRect npcArea;
    if (twoColumns) {
        const int w = (body.width() - gap) / 2;
        bossArea = QRect(body.left(), body.top(), w, body.height());
        npcArea = QRect(body.right() - w + 1, body.top(), w, body.height());
    } else {
        const int h = (body.height() - gap) / 2;
        bossArea = QRect(body.left(), body.top(), body.width(), h);
        npcArea = QRect(body.left(), body.bottom() - h + 1, body.width(), h);
    }
    // Each column's heading stays put; its rows scroll below it.
    const auto drawProgressColumn = [&](const std::vector<Row>& rows, const QRect& area, PaintedScroll& scroll) {
        const std::vector<Row> heading(rows.begin(), rows.begin() + 1);
        const std::vector<Row> entries(rows.begin() + 1, rows.end());
        const QRect list = area.adjusted(0, rowHeight(heading.front(), true) + 4, 0, 0);
        scroll.setGeometry(list, barTrack(list), columnHeight(entries));
        scroll.setStep(3 * lineH);
        drawColumn(heading, area.adjusted(0, 0, -scrollBarW, 0), area.top(), area);
        drawColumn(entries, list.adjusted(0, 0, -scrollBarW, 0), list.top() - scroll.value(), list);
        scroll.paint(painter, trackColor, AREA_COLOR);
    };
    drawProgressColumn(bossRows(c), bossArea, m_scroll);
    drawProgressColumn(npcRows(c), npcArea, m_npcScroll);
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
