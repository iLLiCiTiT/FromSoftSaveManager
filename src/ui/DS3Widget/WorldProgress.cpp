#include "WorldProgress.h"

#include <QPaintEvent>
#include <QPainter>
#include <QWheelEvent>
#include <cmath>
#include <vector>

#include "Painting.h"
#include "../../parse/DS3/WorldProgress.h"

namespace fssm::ui::ds3 {
namespace {
    constexpr int DESIGN_W = 620;
    constexpr int DESIGN_H = 470;
    const QColor AREA_COLOR(214, 190, 136);
    const QColor HOSTILE_COLOR(206, 92, 72);

    enum class RowKind {
        Heading,    // "Bonfires 65/ 78"
        Area,       // travel menu area with its lit count
        Bonfire,
        Boss,
        Npc,
    };

    using fssm::parse::ds3::NpcState;

    struct Row {
        RowKind kind;
        QString text;
        QString value;
        bool done = false;
        NpcState npc = NpcState::Unknown;
    };

    QString count(int done, int total) {
        return QString("%1/ %2").arg(done).arg(total);
    }

    std::vector<Row> bonfireRows(const fssm::parse::ds3::DS3CharacterInfo& c) {
        using fssm::parse::ds3::AREAS;
        using fssm::parse::ds3::BONFIRES;
        const auto isLit = [&c](size_t i) { return i < c.bonfiresLit.size() && c.bonfiresLit[i]; };
        int total = 0;
        for (size_t i = 0; i < BONFIRES.size(); ++i) total += isLit(i);
        std::vector<Row> rows = {{RowKind::Heading, "Bonfires", count(total, int(BONFIRES.size()))}};
        for (size_t area = 0; area < AREAS.size(); ++area) {
            std::vector<Row> entries;
            int lit = 0;
            for (size_t i = 0; i < BONFIRES.size(); ++i) {
                if (BONFIRES[i].area != area) continue;
                const auto& name = BONFIRES[i].name;
                entries.push_back({RowKind::Bonfire, QString::fromUtf8(name.data(), int(name.size())), "", isLit(i)});
                lit += isLit(i);
            }
            if (entries.empty()) continue;
            rows.push_back({RowKind::Area, QString::fromUtf8(AREAS[area].data(), int(AREAS[area].size())),
                            count(lit, int(entries.size()))});
            rows.insert(rows.end(), entries.begin(), entries.end());
        }
        return rows;
    }

    std::vector<Row> bossRows(const fssm::parse::ds3::DS3CharacterInfo& c) {
        using fssm::parse::ds3::BOSSES;
        std::vector<Row> rows;
        int defeated = 0;
        for (size_t i = 0; i < BOSSES.size(); ++i) {
            const bool done = i < c.bossesDefeated.size() && c.bossesDefeated[i];
            const auto& name = BOSSES[i].name;
            rows.push_back({RowKind::Boss, QString::fromUtf8(name.data(), int(name.size())), "", done});
            defeated += done;
        }
        rows.insert(rows.begin(), Row{RowKind::Heading, "Bosses", count(defeated, int(BOSSES.size()))});
        return rows;
    }

    // NPCs by their status flags; the state is shown right aligned (nothing while the
    // NPC's map hasn't loaded yet).
    std::vector<Row> npcRows(const fssm::parse::ds3::DS3CharacterInfo& c) {
        using fssm::parse::ds3::NPCS;
        std::vector<Row> rows = {{RowKind::Heading, "NPCs", ""}};
        for (size_t i = 0; i < NPCS.size(); ++i) {
            const NpcState state = i < c.npcStates.size() ? c.npcStates[i] : NpcState::Unknown;
            QString value;
            switch (state) {
                case NpcState::Friendly: value = "Alive"; break;
                case NpcState::Hostile: value = "Hostile"; break;
                case NpcState::Dead: value = "Dead"; break;
                case NpcState::Unknown: break;
            }
            const auto& name = NPCS[i].name;
            rows.push_back({RowKind::Npc, QString::fromUtf8(name.data(), int(name.size())), value,
                            state == NpcState::Friendly || state == NpcState::Hostile, state});
        }
        return rows;
    }

    // Small circle marking an NPC: filled while alive (red when hostile), crossed out when dead.
    void drawNpcMark(QPainter& painter, const QRectF& rect, NpcState state) {
        painter.save();
        painter.setRenderHint(QPainter::Antialiasing, true);
        const double w = std::max(1.0, rect.width() / 8);
        const QColor edge = state == NpcState::Hostile ? HOSTILE_COLOR
            : state == NpcState::Friendly ? GOLD_COLOR : QColor(110, 104, 96);
        painter.setPen(QPen(edge, w));
        painter.setBrush(state == NpcState::Friendly ? QBrush(QColor(200, 166, 92))
                         : state == NpcState::Hostile ? QBrush(HOSTILE_COLOR.darker(130)) : QBrush(Qt::NoBrush));
        painter.drawEllipse(rect);
        if (state == NpcState::Dead) {
            const QRectF x = rect.adjusted(rect.width() * 0.25, rect.height() * 0.25, -rect.width() * 0.25, -rect.height() * 0.25);
            painter.drawLine(x.topLeft(), x.bottomRight());
            painter.drawLine(x.topRight(), x.bottomLeft());
        }
        painter.restore();
    }

    // Small diamond marking a boss: filled gold when defeated.
    void drawBossMark(QPainter& painter, const QRectF& rect, bool defeated) {
        painter.save();
        painter.setRenderHint(QPainter::Antialiasing, true);
        const QPointF c = rect.center();
        const double r = rect.width() / 2;
        const QPolygonF diamond({{c.x(), c.y() - r}, {c.x() + r, c.y()}, {c.x(), c.y() + r}, {c.x() - r, c.y()}});
        painter.setPen(QPen(defeated ? GOLD_COLOR : QColor(110, 104, 96), std::max(1.0, r / 4)));
        painter.setBrush(defeated ? QBrush(QColor(200, 166, 92)) : QBrush(Qt::NoBrush));
        painter.drawPolygon(diamond);
        painter.restore();
    }
}

WorldProgressWidget::WorldProgressWidget(QWidget* parent): QWidget(parent) {
    setAttribute(Qt::WA_TranslucentBackground, true);
}

void WorldProgressWidget::setCharacter(const fssm::parse::ds3::DS3CharacterInfo* charInfo) {
    const bool sameCharacter = charInfo != nullptr && m_character.has_value() && m_character->index == charInfo->index;
    if (!sameCharacter) m_scroll = 0;
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
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    const double s = std::clamp(std::min(double(width()) / DESIGN_W, double(height()) / DESIGN_H), 0.6, 1.4);
    const auto px = [s](double v) { return int(std::round(v * s)); };

    const QRect panel = rect();
    drawStonePanel(painter, panel);
    const QRect inner = panel.adjusted(px(18), px(8), -px(18), -px(12));
    if (!m_character.has_value()) {
        m_maxScroll = 0;
        painter.setFont(serifFont(px(20)));
        drawShadowText(painter, panel, Qt::AlignCenter, "No character selected", DIM_TEXT_COLOR);
        return;
    }
    const auto& c = *m_character;

    // Title row like the Player Status panel.
    const QRect title(inner.left(), inner.top(), inner.width(), px(32));
    painter.setFont(serifFont(px(21)));
    drawShadowText(painter, title.adjusted(px(4), 0, 0, 0), Qt::AlignLeft | Qt::AlignVCenter, "World Progress");
    drawShadowText(painter, title.adjusted(0, 0, -px(4), 0), Qt::AlignRight | Qt::AlignVCenter,
                   QString::fromStdU16String(c.name));
    drawGoldRule(painter, title.left(), title.right(), title.bottom() + 2, 220);

    const QRect body(inner.left(), title.bottom() + px(10), inner.width(), inner.bottom() - title.bottom() - px(10));
    const int headingH = px(32);
    const int areaH = px(28);
    const int areaGap = px(8);
    const int lineH = px(25);
    const int iconSize = px(20);
    const int scrollBarW = px(10);
    m_lineHeight = lineH;

    const auto rowHeight = [&](const Row& row, bool first) {
        switch (row.kind) {
            case RowKind::Heading: return (first ? 0 : px(16)) + headingH;
            case RowKind::Area: return areaGap + areaH;
            default: return lineH;
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
    const int gap = px(26);
    const bool twoColumns = body.width() >= px(520);
    const int columnWidth = twoColumns ? (body.width() - scrollBarW - gap) / 2 : body.width() - scrollBarW;
    std::vector<Row> single;
    if (!twoColumns) {
        single = bonfires;
        single.insert(single.end(), bosses.begin(), bosses.end());
    }
    const int contentH = twoColumns ? std::max(columnHeight(bonfires), columnHeight(bosses)) : columnHeight(single);
    m_maxScroll = std::max(0, contentH - body.height());
    m_scroll = std::clamp(m_scroll, 0, m_maxScroll);

    painter.save();
    painter.setClipRect(body.adjusted(0, 0, 0, 1), Qt::IntersectClip);
    const auto drawColumn = [&](const std::vector<Row>& rows, int x) {
        int y = body.top() - m_scroll;
        for (size_t i = 0; i < rows.size(); ++i) {
            const Row& row = rows[i];
            const int h = rowHeight(row, i == 0);
            const QRect full(x, y, columnWidth, h);
            y += h;
            if (full.bottom() < body.top() || full.top() > body.bottom()) continue;
            switch (row.kind) {
                case RowKind::Heading: {
                    const QRect line(x, full.bottom() - headingH + 1, columnWidth, headingH);
                    painter.setFont(serifFont(px(20)));
                    drawShadowText(painter, line.adjusted(px(4), 0, 0, 0), Qt::AlignLeft | Qt::AlignVCenter, row.text);
                    drawShadowText(painter, line.adjusted(0, 0, -px(4), 0), Qt::AlignRight | Qt::AlignVCenter, row.value);
                    drawGoldRule(painter, line.left(), line.right(), line.bottom(), 220);
                    break;
                }
                case RowKind::Area: {
                    const QRect line(x, full.bottom() - areaH + 1, columnWidth, areaH);
                    painter.setFont(serifFont(px(17)));
                    const QFontMetrics fm(painter.font());
                    const int valueW = fm.horizontalAdvance(row.value) + px(10);
                    drawShadowText(painter, line.adjusted(px(4), 0, -valueW, 0), Qt::AlignLeft | Qt::AlignVCenter,
                                   row.text, AREA_COLOR);
                    drawShadowText(painter, line.adjusted(0, 0, -px(4), 0), Qt::AlignRight | Qt::AlignVCenter,
                                   row.value, AREA_COLOR);
                    drawGoldRule(painter, line.left(), line.right(), line.bottom() - 1, 140);
                    break;
                }
                case RowKind::Bonfire:
                case RowKind::Boss:
                case RowKind::Npc: {
                    const QRectF icon(x + px(10), full.center().y() - iconSize / 2.0, iconSize, iconSize);
                    const QRectF smallIcon = icon.adjusted(iconSize * 0.2, iconSize * 0.2, -iconSize * 0.2, -iconSize * 0.2);
                    if (row.kind == RowKind::Bonfire) {
                        drawBonfire(painter, icon, row.done);
                    } else if (row.kind == RowKind::Boss) {
                        drawBossMark(painter, smallIcon, row.done);
                    } else {
                        drawNpcMark(painter, smallIcon, row.npc);
                    }
                    painter.setFont(serifFont(px(17)));
                    const int textLeft = int(icon.right()) + px(10);
                    int textRight = full.right();
                    if (!row.value.isEmpty()) {
                        const QFontMetrics fm(painter.font());
                        const QColor valueColor = row.npc == NpcState::Hostile ? HOSTILE_COLOR : DIM_TEXT_COLOR;
                        drawShadowText(painter, QRect(textLeft, full.top(), full.right() - textLeft - px(4), full.height()),
                                       Qt::AlignRight | Qt::AlignVCenter, row.value, valueColor);
                        textRight -= fm.horizontalAdvance(row.value) + px(14);
                    }
                    drawShadowText(painter, QRect(textLeft, full.top(), textRight - textLeft, full.height()),
                                   Qt::AlignLeft | Qt::AlignVCenter, row.text, row.done ? TEXT_COLOR : DIM_TEXT_COLOR);
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

    // Thin scroll bar along the right edge, like the inventory grid's.
    if (m_maxScroll > 0) {
        const QRect track(body.right() - std::max(2, px(3)), body.top(), std::max(2, px(3)), body.height());
        const int thumbH = std::max(16, track.height() * track.height() / (track.height() + m_maxScroll));
        const int thumbY = track.top() + (track.height() - thumbH) * m_scroll / m_maxScroll;
        painter.fillRect(track, QColor(0, 0, 0, 120));
        painter.fillRect(QRect(track.left(), thumbY, track.width(), thumbH), QColor(186, 150, 80));
    }
}

void WorldProgressWidget::wheelEvent(QWheelEvent* event) {
    if (!m_character.has_value()) return;
    const int delta = event->angleDelta().y() * 3 * m_lineHeight / 120;
    const int scroll = std::clamp(m_scroll - delta, 0, m_maxScroll);
    if (scroll != m_scroll) {
        m_scroll = scroll;
        update();
    }
    event->accept();
}
}
