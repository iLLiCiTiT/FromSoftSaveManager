#include "WorldProgress.h"

#include <QLinearGradient>
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QRadialGradient>
#include <QWheelEvent>
#include <cmath>

#include "../../parse/EldenRing/NpcProgress.h"
#include "../../parse/EldenRing/WorldProgress.h"

namespace fssm::ui::er {
namespace {
    // Frame, fonts and rows keep one size (the look at the default window size);
    // resizing only changes how much fits.
    constexpr double FIXED_SCALE = 0.8;

    const QColor TEXT_COLOR(228, 224, 210);
    const QColor DIM_TEXT_COLOR(150, 144, 128);
    const QColor GOLD_COLOR(190, 168, 112);
    const QColor HOSTILE_COLOR(206, 92, 72);

    enum class RowKind {
        Area,       // region with its count
        Grace,
        Boss,
        Npc,
        Step,       // progress step under an NPC
        Ending,
    };

    using fssm::parse::er::EndingState;
    using fssm::parse::er::NpcState;

    struct Row {
        RowKind kind;
        QString text;
        QString value;
        bool done = false;
        NpcState npc = NpcState::Unknown;
        QString detail;     // second, smaller line: an NPC's questline stage
    };

    QFont serifFont(int pixelSize) {
        QFont font("Georgia");
        font.setStyleHint(QFont::Serif);
        font.setPixelSize(std::max(8, pixelSize));
        return font;
    }

    void drawText(QPainter& painter, const QRect& rect, int flags, const QString& text,
                  const QColor& color = TEXT_COLOR) {
        const QFontMetrics fm(painter.font());
        const QString elided = fm.elidedText(text, Qt::ElideRight, rect.width());
        const int offset = std::max(1, painter.font().pixelSize() / 14);
        painter.save();
        painter.setPen(QColor(0, 0, 0, 210));
        painter.drawText(rect.translated(offset, offset), flags, elided);
        painter.setPen(color);
        painter.drawText(rect, flags, elided);
        painter.restore();
    }

    // Thin rule that fades out to the right, like the lines under the game's headings.
    void drawRule(QPainter& painter, int x1, int x2, int y, int alpha = 150) {
        if (x2 <= x1) return;
        QLinearGradient gradient(x1, 0, x2, 0);
        QColor color = GOLD_COLOR;
        color.setAlpha(alpha);
        QColor clear = color;
        clear.setAlpha(0);
        gradient.setColorAt(0.0, color);
        gradient.setColorAt(0.7, color);
        gradient.setColorAt(1.0, clear);
        painter.save();
        painter.setPen(QPen(QBrush(gradient), 1));
        painter.drawLine(x1, y, x2, y);
        painter.restore();
    }

    // Translucent dark backdrop the game puts behind its menu panes.
    void drawPane(QPainter& painter, const QRect& rect) {
        painter.save();
        QLinearGradient gradient(rect.topLeft(), rect.topRight());
        gradient.setColorAt(0.0, QColor(10, 10, 8, 150));
        gradient.setColorAt(0.85, QColor(10, 10, 8, 110));
        gradient.setColorAt(1.0, QColor(10, 10, 8, 40));
        painter.setPen(Qt::NoPen);
        painter.setBrush(gradient);
        painter.drawRect(rect);
        painter.restore();
    }

    QString fromView(std::string_view text) {
        return QString::fromUtf8(text.data(), int(text.size()));
    }

    QString count(int done, int total) {
        return QString("%1/ %2").arg(done).arg(total);
    }

    bool isSet(const std::vector<bool>& values, size_t i) {
        return i < values.size() && values[i];
    }

    // Rows of one list grouped by area, and how many of its entries are done.
    template <typename Entries, typename Areas>
    std::vector<Row> groupedRows(const Entries& entries, const Areas& areas, const std::vector<bool>& done,
                                 RowKind kind, int& doneCount) {
        std::vector<Row> rows;
        doneCount = 0;
        for (size_t area = 0; area < areas.size(); ++area) {
            std::vector<Row> group;
            int areaDone = 0;
            for (size_t i = 0; i < entries.size(); ++i) {
                if (entries[i].area != area) continue;
                group.push_back({kind, fromView(entries[i].name), "", isSet(done, i)});
                areaDone += isSet(done, i);
            }
            if (group.empty()) continue;
            rows.push_back({RowKind::Area, fromView(areas[area]), count(areaDone, int(group.size()))});
            rows.insert(rows.end(), group.begin(), group.end());
            doneCount += areaDone;
        }
        return rows;
    }

    // NPCs by their status flags under an "NPCs" heading; the state is shown right aligned
    // (nothing while the NPC's map hasn't loaded yet), the questline stage below the name
    // and the NPC's progress steps (NPC_STEPS) as a checklist under it.
    std::vector<Row> npcRows(const std::vector<NpcState>& states, const std::vector<std::string_view>& stages,
                             const std::vector<bool>& stepsDone) {
        using fssm::parse::er::NPC_STEPS;
        using fssm::parse::er::NPCS;
        std::vector<Row> rows = {{RowKind::Area, "NPCs", ""}};
        for (size_t i = 0; i < NPCS.size(); ++i) {
            const NpcState state = i < states.size() ? states[i] : NpcState::Unknown;
            QString value;
            switch (state) {
                case NpcState::Friendly: value = "Alive"; break;
                case NpcState::Hostile: value = "Hostile"; break;
                case NpcState::Dead: value = "Dead"; break;
                case NpcState::Unknown: break;
            }
            rows.push_back({RowKind::Npc, fromView(NPCS[i].name), value,
                            state == NpcState::Friendly || state == NpcState::Hostile, state,
                            i < stages.size() ? fromView(stages[i]) : QString()});
            for (size_t k = 0; k < NPC_STEPS.size(); ++k) {
                if (NPC_STEPS[k].statusFlag != NPCS[i].statusFlag) continue;
                rows.push_back({RowKind::Step, fromView(NPC_STEPS[k].text), "", isSet(stepsDone, k)});
            }
        }
        return rows;
    }

    // Endings under their own heading: reached, or available to choose.
    std::vector<Row> endingRows(const std::vector<EndingState>& endings) {
        using fssm::parse::er::ENDINGS;
        std::vector<Row> rows = {{RowKind::Area, "Endings", ""}};
        int reached = 0;
        for (size_t i = 0; i < ENDINGS.size(); ++i) {
            const EndingState state = i < endings.size() ? endings[i] : EndingState::Locked;
            rows.push_back({RowKind::Ending, fromView(ENDINGS[i].name),
                            state == EndingState::Reached ? "Reached"
                            : state == EndingState::Available ? "Available" : "",
                            state == EndingState::Reached});
            reached += state == EndingState::Reached;
        }
        rows[0].value = count(reached, int(ENDINGS.size()));
        return rows;
    }

    // Small box of a checklist step, ticked when done.
    void drawStepMark(QPainter& painter, const QRectF& rect, bool done) {
        painter.save();
        const QRectF box = rect.adjusted(rect.width() * 0.2, rect.height() * 0.2,
                                         -rect.width() * 0.2, -rect.height() * 0.2);
        const double w = std::max(1.0, box.width() / 7);
        painter.setPen(QPen(done ? GOLD_COLOR : QColor(110, 104, 92), w));
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(box);
        if (done) {
            QPainterPath tick;
            tick.moveTo(box.left() + box.width() * 0.15, box.top() + box.height() * 0.5);
            tick.lineTo(box.left() + box.width() * 0.42, box.top() + box.height() * 0.8);
            tick.lineTo(box.left() + box.width() * 0.95, box.top() - box.height() * 0.1);
            painter.setPen(QPen(QColor(232, 204, 130), w * 1.6, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            painter.drawPath(tick);
        }
        painter.restore();
    }

    // Small circle marking an NPC: filled while alive (red when hostile), crossed out when dead.
    void drawNpcMark(QPainter& painter, const QRectF& rect, NpcState state) {
        painter.save();
        const QRectF circle = rect.adjusted(rect.width() * 0.22, rect.height() * 0.22,
                                            -rect.width() * 0.22, -rect.height() * 0.22);
        const double w = std::max(1.0, circle.width() / 8);
        const QColor edge = state == NpcState::Hostile ? HOSTILE_COLOR
            : state == NpcState::Friendly ? GOLD_COLOR : QColor(110, 104, 92);
        painter.setPen(QPen(edge, w));
        painter.setBrush(state == NpcState::Friendly ? QBrush(QColor(214, 184, 112))
                         : state == NpcState::Hostile ? QBrush(HOSTILE_COLOR.darker(130)) : QBrush(Qt::NoBrush));
        painter.drawEllipse(circle);
        if (state == NpcState::Dead) {
            const QRectF x = circle.adjusted(circle.width() * 0.25, circle.height() * 0.25,
                                             -circle.width() * 0.25, -circle.height() * 0.25);
            painter.drawLine(x.topLeft(), x.bottomRight());
            painter.drawLine(x.topRight(), x.bottomLeft());
        }
        painter.restore();
    }

    // Site of Grace: a golden glow with a spark when discovered, a faint ring otherwise.
    void drawGraceMark(QPainter& painter, const QRectF& rect, bool discovered) {
        painter.save();
        const QPointF c = rect.center();
        const double r = rect.width() / 2;
        if (discovered) {
            QRadialGradient glow(c, r);
            glow.setColorAt(0.0, QColor(255, 244, 200, 255));
            glow.setColorAt(0.3, QColor(232, 196, 110, 220));
            glow.setColorAt(1.0, QColor(200, 160, 70, 0));
            painter.setPen(Qt::NoPen);
            painter.setBrush(glow);
            painter.drawEllipse(c, r, r);
            QPainterPath spark;
            spark.moveTo(c.x(), c.y() - r * 0.85);
            spark.quadTo(c.x() + r * 0.12, c.y() - r * 0.12, c.x() + r * 0.85, c.y());
            spark.quadTo(c.x() + r * 0.12, c.y() + r * 0.12, c.x(), c.y() + r * 0.85);
            spark.quadTo(c.x() - r * 0.12, c.y() + r * 0.12, c.x() - r * 0.85, c.y());
            spark.quadTo(c.x() - r * 0.12, c.y() - r * 0.12, c.x(), c.y() - r * 0.85);
            painter.setBrush(QColor(255, 248, 220, 230));
            painter.drawPath(spark);
        } else {
            painter.setPen(QPen(QColor(110, 104, 92), std::max(1.0, r / 6)));
            painter.setBrush(Qt::NoBrush);
            painter.drawEllipse(c, r * 0.45, r * 0.45);
        }
        painter.restore();
    }

    // Small diamond marking a boss: filled gold when defeated.
    void drawBossMark(QPainter& painter, const QRectF& rect, bool defeated) {
        painter.save();
        const QPointF c = rect.center();
        const double r = rect.width() / 2 * 0.62;
        const QPolygonF diamond({{c.x(), c.y() - r}, {c.x() + r, c.y()}, {c.x(), c.y() + r}, {c.x() - r, c.y()}});
        painter.setPen(QPen(defeated ? GOLD_COLOR : QColor(110, 104, 92), std::max(1.0, r / 4)));
        painter.setBrush(defeated ? QBrush(QColor(214, 184, 112)) : QBrush(Qt::NoBrush));
        painter.drawPolygon(diamond);
        painter.restore();
    }
}

WorldProgressWidget::WorldProgressWidget(QWidget* parent): QWidget(parent) {
    setAttribute(Qt::WA_TranslucentBackground, true);
}

void WorldProgressWidget::setCharacter(const fssm::parse::er::ERCharacterInfo* charInfo) {
    const int index = charInfo != nullptr ? charInfo->index : -1;
    if (index != m_index) m_scroll = {};
    m_index = index;
    m_hasCharacter = charInfo != nullptr;
    m_gracesDiscovered = charInfo != nullptr ? charInfo->gracesDiscovered : std::vector<bool>{};
    m_bossesDefeated = charInfo != nullptr ? charInfo->bossesDefeated : std::vector<bool>{};
    m_npcStates = charInfo != nullptr ? charInfo->npcStates : std::vector<NpcState>{};
    m_npcStages = charInfo != nullptr ? charInfo->npcStages : std::vector<std::string_view>{};
    m_npcStepsDone = charInfo != nullptr ? charInfo->npcStepsDone : std::vector<bool>{};
    m_endings = charInfo != nullptr ? charInfo->endings : std::vector<EndingState>{};
    update();
}

void WorldProgressWidget::paintEvent(QPaintEvent* event) {
    using fssm::parse::er::BOSS_AREAS;
    using fssm::parse::er::BOSSES;
    using fssm::parse::er::GRACE_AREAS;
    using fssm::parse::er::GRACES;

    QPainter painter(this);
    painter.setClipRect(event->rect());
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    const QRect r = rect();
    const double s = FIXED_SCALE;
    const auto px = [s](double v) { return int(std::round(v * s)); };

    if (!m_hasCharacter || m_gracesDiscovered.empty()) {
        m_maxScroll = {};
        painter.setFont(serifFont(px(18)));
        drawText(painter, r, Qt::AlignCenter,
                 m_hasCharacter ? "World progress is not available for this character" : "No character selected",
                 DIM_TEXT_COLOR);
        return;
    }

    int graceCount = 0;
    int bossCount = 0;
    const std::vector<Row> graces = groupedRows(GRACES, GRACE_AREAS, m_gracesDiscovered, RowKind::Grace, graceCount);
    std::vector<Row> bosses = groupedRows(BOSSES, BOSS_AREAS, m_bossesDefeated, RowKind::Boss, bossCount);
    const std::vector<Row> npcs = npcRows(m_npcStates, m_npcStages, m_npcStepsDone);
    bosses.insert(bosses.end(), npcs.begin(), npcs.end());
    const std::vector<Row> endings = endingRows(m_endings);
    bosses.insert(bosses.end(), endings.begin(), endings.end());

    const int gap = px(16);
    const int columnW = (r.width() - gap) / 2;
    const int titleH = px(34);
    const int areaH = px(28);
    const int lineH = px(24);
    const int detailH = px(16);
    const int stepH = px(19);
    const int iconSize = px(18);
    m_lineHeight = lineH;

    const auto rowHeight = [&](const Row& row, bool first) {
        return row.kind == RowKind::Area ? (first ? 0 : px(8)) + areaH
             : row.kind == RowKind::Step ? stepH
             : row.detail.isEmpty() ? lineH : lineH + detailH;
    };
    const auto drawColumn = [&](const std::vector<Row>& rows, int column, const QString& title, const QString& value) {
        const QRect pane(r.left() + column * (columnW + gap), r.top(), columnW, r.height());
        drawPane(painter, pane);
        const QRect titleRect(pane.left() + px(12), pane.top() + px(6), pane.width() - px(24), titleH);
        painter.setFont(serifFont(px(19)));
        drawText(painter, titleRect, Qt::AlignLeft | Qt::AlignVCenter, title);
        drawText(painter, titleRect, Qt::AlignRight | Qt::AlignVCenter, value);
        drawRule(painter, titleRect.left(), titleRect.right(), titleRect.bottom(), 200);

        const QRect list(pane.left() + px(8), titleRect.bottom() + px(8), pane.width() - px(16),
                         std::max(0, pane.bottom() - titleRect.bottom() - px(14)));
        m_lists[column] = list;
        int contentH = 0;
        for (size_t i = 0; i < rows.size(); ++i) contentH += rowHeight(rows[i], i == 0);
        m_maxScroll[column] = std::max(0, contentH - list.height());
        m_scroll[column] = std::clamp(m_scroll[column], 0, m_maxScroll[column]);

        painter.save();
        painter.setClipRect(list.adjusted(0, 0, 0, 1), Qt::IntersectClip);
        const int scrollBarW = m_maxScroll[column] > 0 ? px(10) : 0;
        int y = list.top() - m_scroll[column];
        for (size_t i = 0; i < rows.size(); ++i) {
            const Row& row = rows[i];
            const int h = rowHeight(row, i == 0);
            const QRect full(list.left(), y, list.width() - scrollBarW, h);
            y += h;
            if (full.bottom() < list.top() || full.top() > list.bottom()) continue;
            if (row.kind == RowKind::Area) {
                const QRect line(full.left(), full.bottom() - areaH + 1, full.width(), areaH);
                painter.setFont(serifFont(px(16)));
                const int valueW = QFontMetrics(painter.font()).horizontalAdvance(row.value) + px(10);
                drawText(painter, line.adjusted(px(4), 0, -valueW, 0), Qt::AlignLeft | Qt::AlignVCenter, row.text,
                         GOLD_COLOR);
                drawText(painter, line.adjusted(0, 0, -px(4), 0), Qt::AlignRight | Qt::AlignVCenter, row.value,
                         GOLD_COLOR);
                drawRule(painter, line.left(), line.right(), line.bottom() - 1, 110);
                continue;
            }
            if (row.kind == RowKind::Step) {
                // A smaller line per step, indented to the NPC's name.
                const int left = full.left() + px(10) + iconSize + px(10);
                const QRectF box(left, full.center().y() - px(14) / 2.0, px(14), px(14));
                drawStepMark(painter, box, row.done);
                painter.setFont(serifFont(px(13)));
                const int textLeft = int(box.right()) + px(6);
                drawText(painter, QRect(textLeft, full.top(), full.right() - textLeft - px(4), full.height()),
                         Qt::AlignLeft | Qt::AlignVCenter, row.text, row.done ? TEXT_COLOR : DIM_TEXT_COLOR);
                continue;
            }
            const QRect line(full.left(), full.top(), full.width(), lineH);
            const QRectF icon(line.left() + px(10), line.center().y() - iconSize / 2.0, iconSize, iconSize);
            if (row.kind == RowKind::Grace) {
                drawGraceMark(painter, icon, row.done);
            } else if (row.kind == RowKind::Boss || row.kind == RowKind::Ending) {
                drawBossMark(painter, icon, row.done);
            } else {
                drawNpcMark(painter, icon, row.npc);
            }
            painter.setFont(serifFont(px(15)));
            const int textLeft = int(icon.right()) + px(10);
            int textRight = line.right();
            if (!row.value.isEmpty()) {
                drawText(painter, QRect(textLeft, line.top(), line.right() - textLeft - px(4), line.height()),
                         Qt::AlignRight | Qt::AlignVCenter, row.value,
                         row.npc == NpcState::Hostile ? HOSTILE_COLOR
                         : row.kind == RowKind::Ending && !row.done ? GOLD_COLOR : DIM_TEXT_COLOR);
                textRight -= QFontMetrics(painter.font()).horizontalAdvance(row.value) + px(14);
            }
            drawText(painter, QRect(textLeft, line.top(), textRight - textLeft, line.height()),
                     Qt::AlignLeft | Qt::AlignVCenter, row.text, row.done ? TEXT_COLOR : DIM_TEXT_COLOR);
            if (!row.detail.isEmpty()) {
                painter.setFont(serifFont(px(12)));
                drawText(painter, QRect(textLeft, line.bottom() - px(3), full.right() - textLeft - px(4), detailH),
                         Qt::AlignLeft | Qt::AlignTop, row.detail, row.done ? GOLD_COLOR : DIM_TEXT_COLOR);
            }
        }
        painter.restore();

        // Thin scroll bar along the right edge, like the inventory grid's.
        if (m_maxScroll[column] > 0) {
            const QRect track(list.right() - std::max(2, px(3)), list.top(), std::max(2, px(3)), list.height());
            const int thumbH = std::max(16, track.height() * track.height() / (track.height() + m_maxScroll[column]));
            const int thumbY = track.top() + (track.height() - thumbH) * m_scroll[column] / m_maxScroll[column];
            painter.fillRect(track, QColor(0, 0, 0, 120));
            painter.fillRect(QRect(track.left(), thumbY, track.width(), thumbH), GOLD_COLOR);
        }
    };
    drawColumn(graces, 0, "Sites of Grace", count(graceCount, int(GRACES.size())));
    drawColumn(bosses, 1, "Bosses", count(bossCount, int(BOSSES.size())));
}

void WorldProgressWidget::wheelEvent(QWheelEvent* event) {
    const int column = event->position().toPoint().x() < m_lists[1].left() ? 0 : 1;
    const int delta = event->angleDelta().y() * 3 * m_lineHeight / 120;
    const int scroll = std::clamp(m_scroll[column] - delta, 0, m_maxScroll[column]);
    if (scroll != m_scroll[column]) {
        m_scroll[column] = scroll;
        update();
    }
    event->accept();
}
}
