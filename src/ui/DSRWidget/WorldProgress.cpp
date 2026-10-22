#include "WorldProgress.h"

#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QWheelEvent>
#include <algorithm>
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
    // map hasn't loaded yet) and the questline stage below the name.
    std::vector<Row> npcRows(const fssm::parse::dsr::DSRCharacterInfo& c) {
        using fssm::parse::dsr::NPCS;
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

WorldProgressWidget::WorldProgressWidget(QWidget* parent): QWidget(parent) {
    setAttribute(Qt::WA_TranslucentBackground, true);
    m_scroll.setContentDrag(true);
}

void WorldProgressWidget::setCharacter(const fssm::parse::dsr::DSRCharacterInfo* charInfo) {
    const bool sameCharacter = charInfo != nullptr && m_character.has_value() && m_character->index == charInfo->index;
    if (!sameCharacter) m_scroll.reset();
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
    const int areaH = lineH + 2;
    const int areaGap = 8;
    const int headingH = QFontMetrics(headingFont).height() + 10;
    const int iconSize = fm.height() - 2;
    const int scrollBarW = 10;

    const auto rowHeight = [&](const Row& row, bool first) {
        switch (row.kind) {
            case RowKind::Heading: return (first ? 0 : 16) + headingH;
            case RowKind::Area: return areaGap + areaH;
            default: return row.detail.isEmpty() ? lineH : lineH + detailH;
        }
    };
    const auto columnHeight = [&](const std::vector<Row>& rows) {
        int h = 0;
        for (size_t i = 0; i < rows.size(); ++i) h += rowHeight(rows[i], i == 0);
        return h;
    };

    const std::vector<Row> bonfires = bonfireRows(c);
    std::vector<Row> bosses = bossRows(c);
    const std::vector<Row> npcs = npcRows(c);
    bosses.insert(bosses.end(), npcs.begin(), npcs.end());
    const int gap = 24;
    const bool twoColumns = body.width() >= 480;
    const int columnWidth = twoColumns ? (body.width() - scrollBarW - gap) / 2 : body.width() - scrollBarW;
    std::vector<Row> single;
    if (!twoColumns) {
        single = bonfires;
        single.insert(single.end(), bosses.begin(), bosses.end());
    }
    const int contentH = twoColumns ? std::max(columnHeight(bonfires), columnHeight(bosses)) : columnHeight(single);
    m_scroll.setGeometry(body, QRect(body.right() - 3, body.top(), 3, body.height()), contentH);
    m_scroll.setStep(3 * lineH);

    painter.save();
    painter.setClipRect(body.adjusted(0, 0, 0, 1), Qt::IntersectClip);
    const auto drawRule = [&](int left, int right, int y) {
        painter.setPen(QPen(RULE_COLOR, 1));
        painter.drawLine(left, y, right, y);
    };
    const auto drawColumn = [&](const std::vector<Row>& rows, int x) {
        int y = body.top() - m_scroll.value();
        for (size_t i = 0; i < rows.size(); ++i) {
            const Row& row = rows[i];
            const int h = rowHeight(row, i == 0);
            const QRect full(x, y, columnWidth, h);
            y += h;
            if (full.bottom() < body.top() || full.top() > body.bottom()) continue;
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
    };
    if (twoColumns) {
        drawColumn(bonfires, body.left());
        drawColumn(bosses, body.left() + columnWidth + gap);
    } else {
        drawColumn(single, body.left());
    }
    painter.restore();

    // Thin scroll bar along the right edge.
    m_scroll.paint(painter, QColor(0, 0, 0, 120), AREA_COLOR);
}

void WorldProgressWidget::wheelEvent(QWheelEvent* event) {
    if (!m_character.has_value()) return;
    m_scroll.wheel(event, true);
}

void WorldProgressWidget::mousePressEvent(QMouseEvent* event) {
    if (!m_scroll.mousePress(event)) QWidget::mousePressEvent(event);
}

void WorldProgressWidget::mouseMoveEvent(QMouseEvent* event) {
    if (!m_scroll.mouseMove(event)) QWidget::mouseMoveEvent(event);
}

void WorldProgressWidget::mouseReleaseEvent(QMouseEvent* event) {
    if (!m_scroll.mouseRelease(event)) QWidget::mouseReleaseEvent(event);
}

void WorldProgressWidget::leaveEvent(QEvent* event) {
    m_scroll.leave();
    QWidget::leaveEvent(event);
}
}
