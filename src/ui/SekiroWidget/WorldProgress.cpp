#include "WorldProgress.h"

#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QRadialGradient>
#include <QWheelEvent>
#include <climits>
#include <cstdlib>

#include "../../parse/Sekiro/WorldProgress.h"

namespace fssm::ui::sekiro {
namespace {
    // Frame, fonts and rows keep one size (the look at the default window size);
    // resizing only changes how much fits.
    constexpr double FIXED_SCALE = 0.89;

    enum class RowKind {
        Area,       // travel menu area or panel section with its count
        Idol,
        Boss,
        Tool,       // prosthetic tool
        Npc,        // name, state on the right
        Stage,      // the NPC's questline stage
        Step,       // a progress step of the NPC
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

    // Prosthetic tools found, under their own heading.
    std::vector<Row> toolRows(const std::vector<bool>& toolsFound) {
        using fssm::parse::sekiro::PROSTHETIC_TOOLS;
        std::vector<Row> rows;
        int found = 0;
        for (size_t i = 0; i < PROSTHETIC_TOOLS.size(); ++i) found += isSet(toolsFound, i);
        rows.push_back({RowKind::Area, "Prosthetic Tools", count(found, int(PROSTHETIC_TOOLS.size()))});
        for (size_t i = 0; i < PROSTHETIC_TOOLS.size(); ++i) {
            rows.push_back({RowKind::Tool, label(PROSTHETIC_TOOLS[i].name), "", isSet(toolsFound, i)});
        }
        return rows;
    }

    // Every NPC with its state, current stage and steps; met counts the NPCs whose state is known.
    std::vector<Row> npcRows(const std::vector<fssm::parse::sekiro::NpcState>& states,
                             const std::vector<std::string_view>& stages, const std::vector<bool>& dragonrot,
                             const std::vector<bool>& stepsDone, int& met) {
        using fssm::parse::sekiro::NPC_STEPS;
        using fssm::parse::sekiro::NPCS;
        using fssm::parse::sekiro::NpcState;
        std::vector<Row> rows;
        met = 0;
        for (size_t i = 0; i < NPCS.size(); ++i) met += i < states.size() && states[i] != NpcState::Unknown;
        for (size_t i = 0; i < NPCS.size(); ++i) {
            const NpcState state = i < states.size() ? states[i] : NpcState::Unknown;
            const QString value = state == NpcState::Dead      ? "Dead or gone"
                                : state == NpcState::Hostile   ? "Hostile"
                                : isSet(dragonrot, i)          ? "Dragonrot"
                                : state == NpcState::Unknown   ? "Not met"
                                                               : "";
            rows.push_back({RowKind::Npc, label(NPCS[i].name), value, state != NpcState::Unknown});
            if (i < stages.size() && !stages[i].empty()) rows.push_back({RowKind::Stage, label(stages[i])});
            for (size_t s = 0; s < NPC_STEPS.size(); ++s) {
                if (NPC_STEPS[s].npcFlag != NPCS[i].firstFlag) continue;
                rows.push_back({RowKind::Step, label(NPC_STEPS[s].text), "", isSet(stepsDone, s)});
            }
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

WorldProgressWidget::WorldProgressWidget(WorldPage page, QWidget* parent): QWidget(parent), m_page(page) {
    setAttribute(Qt::WA_TranslucentBackground, true);
    for (auto& scroll : m_scrolls) scroll.setContentDrag(true);
}

void WorldProgressWidget::setProgress(const fssm::parse::sekiro::SekiroCharacterInfo& character) {
    if (character.idolsFound != m_idolsFound || character.bossesDefeated != m_bossesDefeated
        || character.npcStepsDone != m_npcStepsDone || character.npcStages != m_npcStages
        || character.npcDragonrot != m_npcDragonrot) {
        for (auto& scroll : m_scrolls) scroll.reset();
    }
    m_idolsFound = character.idolsFound;
    m_bossesDefeated = character.bossesDefeated;
    m_toolsFound = character.prostheticToolsFound;
    m_npcStates = character.npcStates;
    m_npcStages = character.npcStages;
    m_npcDragonrot = character.npcDragonrot;
    m_npcStepsDone = character.npcStepsDone;
    update();
}

void WorldProgressWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    m_cache.clear();
}

void WorldProgressWidget::paintEvent(QPaintEvent* event) {
    using fssm::parse::sekiro::BOSSES;
    using fssm::parse::sekiro::IDOLS;
    using fssm::parse::sekiro::NPCS;

    QPainter painter(this);
    painter.setClipRect(event->rect());
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    const QRect r = rect();
    const double s = FIXED_SCALE;
    const auto px = [s](double v) { return int(std::round(v * s)); };

    const int leftW = int(r.width() * 0.52);
    const int headerH = px(22);
    const int areaH = px(30);
    const int lineH = px(26);
    const int iconSize = px(20);
    const int stageH = px(19);
    const int stepH = px(21);

    // Label over the board, like the inventory's category label.
    const auto drawBoardHeader = [&](const QRect& header, const QString& text) {
        painter.setOpacity(0.55);
        const QRect headerBar = header.adjusted(0, 0, -header.width() / 3, 0);
        painter.drawPixmap(headerBar.topLeft(), m_cache.get(":/sekiro_images/header_bar", headerBar.size()));
        painter.setOpacity(1.0);
        painter.setFont(serifFont(px(16)));
        drawShadowText(painter, header.adjusted(px(14), 0, 0, 0), Qt::AlignLeft | Qt::AlignVCenter, text);
    };

    const auto rowHeight = [&](const Row& row, bool first) {
        switch (row.kind) {
            case RowKind::Area: return (first ? 0 : px(6)) + areaH;
            case RowKind::Stage: return stageH;
            case RowKind::Step: return stepH;
            default: return lineH;
        }
    };
    const auto columnHeight = [&](const std::vector<Row>& rows) {
        int h = 0;
        for (size_t i = 0; i < rows.size(); ++i) h += rowHeight(rows[i], i == 0);
        return h;
    };
    // Rows on the board (dark ink) or in the detail panel (light text); a long list on the
    // board may be split into columns side by side, scrolling together.
    const auto drawRows = [&](const std::vector<std::vector<Row>>& columns, int scrollIndex, const QRect& list,
                              bool board) {
        PaintedScroll& scroll = m_scrolls[scrollIndex];
        int contentH = 0;
        for (const auto& rows : columns) contentH = std::max(contentH, columnHeight(rows));
        const int barW = std::max(2, px(3));
        scroll.setGeometry(list, QRect(list.right() - barW, list.top() + px(4), barW, std::max(0, list.height() - px(8))),
                           contentH);
        scroll.setStep(3 * lineH);
        scroll.setMinimumThumbHeight(px(18));

        painter.save();
        painter.setClipRect(list, Qt::IntersectClip);
        const int gap = px(16);
        const int n = int(columns.size());
        const int columnW = n == 1 ? list.width() : (list.width() - px(10) - gap * (n - 1)) / n;
        for (int c = 0; c < n; ++c) {
            const std::vector<Row>& rows = columns[c];
            int y = list.top() - scroll.value();
            for (size_t i = 0; i < rows.size(); ++i) {
                const Row& row = rows[i];
                const int h = rowHeight(row, i == 0);
                const QRect full(list.left() + c * (columnW + gap), y, columnW, h);
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
                if (row.kind == RowKind::Npc) {
                    // Name with the state on the right, like a boss row without its seal.
                    painter.setFont(serifFont(px(13)));
                    const QFontMetrics fm(painter.font());
                    const int valueW = row.value.isEmpty() ? 0 : fm.horizontalAdvance(row.value) + px(10);
                    const QRect value(full.left(), full.top(), full.width() - px(12), full.height());
                    // Hostile in red, Dragonrot in ochre.
                    const QColor valueColor = row.value == "Hostile"     ? QColor(222, 120, 96)
                                            : row.value == "Dragonrot"   ? QColor(214, 176, 92)
                                                                         : QColor(150, 144, 130);
                    drawShadowText(painter, value, Qt::AlignRight | Qt::AlignVCenter, row.value, valueColor);
                    painter.setFont(serifFont(px(15)));
                    const QRect text(full.left() + px(22), full.top(), full.width() - px(34) - valueW, full.height());
                    drawShadowText(painter, text, Qt::AlignLeft | Qt::AlignVCenter,
                                   QFontMetrics(painter.font()).elidedText(row.text, Qt::ElideRight, text.width()),
                                   row.done ? TEXT_COLOR : QColor(150, 144, 130));
                    continue;
                }
                if (row.kind == RowKind::Stage || row.kind == RowKind::Step) {
                    // Under the NPC's name: its current stage, then a small seal per step.
                    painter.setFont(serifFont(px(13)));
                    int textLeft = full.left() + px(34);
                    QColor color = DIM_TEXT_COLOR;
                    if (row.kind == RowKind::Step) {
                        const double markSize = iconSize * 0.7;
                        drawBossMark(painter, QRectF(textLeft, full.center().y() - markSize / 2.0, markSize, markSize), row.done);
                        textLeft += int(markSize) + px(7);
                        color = row.done ? TEXT_COLOR : QColor(150, 144, 130);
                    }
                    const QRect text(textLeft, full.top(), full.right() - px(10) - textLeft, full.height());
                    drawShadowText(painter, text, Qt::AlignLeft | Qt::AlignVCenter,
                                   QFontMetrics(painter.font()).elidedText(row.text, Qt::ElideRight, text.width()), color);
                    continue;
                }
                const QRectF icon(full.left() + px(22), full.center().y() - iconSize / 2.0, iconSize, iconSize);
                if (row.kind == RowKind::Idol || row.kind == RowKind::Tool) {
                    drawIdolMark(painter, icon, row.done);
                } else {
                    drawBossMark(painter, icon, row.done);
                }
                painter.setFont(serifFont(px(15)));
                const int textLeft = int(icon.right()) + px(10);
                // Dark ink on the board, light text in the panel, like the game.
                const QColor color = board ? (row.done ? QColor(26, 24, 20) : QColor(26, 24, 20, 120))
                                                 : (row.done ? TEXT_COLOR : QColor(150, 144, 130));
                painter.setPen(color);
                const QRect text(textLeft, full.top(), full.right() - textLeft, full.height());
                if (board) {
                    painter.drawText(text, Qt::AlignLeft | Qt::AlignVCenter,
                                     QFontMetrics(painter.font()).elidedText(row.text, Qt::ElideRight, text.width()));
                } else {
                    drawShadowText(painter, text, Qt::AlignLeft | Qt::AlignVCenter, row.text, color);
                }
            }
        }
        painter.restore();

        scroll.paint(painter, QColor(30, 28, 24, 120), QColor(236, 230, 214), 1);
    };

    if (m_page == WorldPage::Idols) {
        // The whole board, the areas in two balanced columns when it is wide enough.
        m_scrolls[1].clear();
        int idolCount = 0;
        const std::vector<Row> idols = groupedRows(IDOLS, m_idolsFound, RowKind::Idol, idolCount);
        const QRect header(r.left() + px(14), r.top(), leftW - px(28), headerH);
        drawBoardHeader(header, QString("Sculptor's Idols  (%1)").arg(count(idolCount, int(IDOLS.size()))));
        const QRect list(r.left(), header.bottom() + 1 + px(8), r.width() - px(16),
                         std::max(0, r.bottom() - header.bottom() - px(10)));
        std::vector<std::vector<Row>> columns = {idols};
        if (r.width() >= px(560)) {
            // Split at the area start closest to the middle.
            const int half = columnHeight(idols) / 2;
            size_t split = 0;
            int best = INT_MAX;
            int y = 0;
            for (size_t i = 0; i < idols.size(); ++i) {
                if (idols[i].kind == RowKind::Area && i > 0 && std::abs(y - half) < best) {
                    best = std::abs(y - half);
                    split = i;
                }
                y += rowHeight(idols[i], i == 0);
            }
            if (split > 0) {
                columns = {std::vector<Row>(idols.begin(), idols.begin() + split),
                           std::vector<Row>(idols.begin() + split, idols.end())};
            }
        }
        drawRows(columns, 0, list, true);
        return;
    }

    // Left: bosses and prosthetic tools on the board like the item list. Right: NPCs in
    // the detail panel.
    int bossCount = 0;
    std::vector<Row> bosses = groupedRows(BOSSES, m_bossesDefeated, RowKind::Boss, bossCount);
    const std::vector<Row> tools = toolRows(m_toolsFound);
    bosses.insert(bosses.end(), tools.begin(), tools.end());
    int npcsMet = 0;
    const std::vector<Row> npcs = npcRows(m_npcStates, m_npcStages, m_npcDragonrot, m_npcStepsDone, npcsMet);

    const QRect bossHeader(r.left() + px(14), r.top(), leftW - px(28), headerH);
    drawBoardHeader(bossHeader, QString("Bosses  (%1)").arg(count(bossCount, int(BOSSES.size()))));
    const QRect panel(r.left() + leftW + px(4), r.top() - px(4), r.width() - leftW - px(4), r.height() + px(4));
    painter.drawPixmap(panel.topLeft(), m_cache.get(":/sekiro_images/detail_panel", panel.size()));
    const QRect panelInner = panel.adjusted(px(14), px(12), -px(14), -px(14));
    const QRect npcTitle(panelInner.left(), panelInner.top(), panelInner.width(), px(32));
    painter.drawPixmap(npcTitle.topLeft(), m_cache.get(":/sekiro_images/title_bar", npcTitle.size()));
    painter.setFont(serifFont(px(18)));
    drawShadowText(painter, npcTitle.adjusted(px(24), 0, -px(24), 0), Qt::AlignCenter,
                   QString("NPCs  (%1)").arg(count(npcsMet, int(NPCS.size()))));

    drawRows({bosses}, 0, QRect(r.left(), bossHeader.bottom() + 1 + px(8), leftW - px(16),
                                std::max(0, r.bottom() - bossHeader.bottom() - px(10))), true);
    drawRows({npcs}, 1, QRect(panelInner.left(), npcTitle.bottom() + 1 + px(6), panelInner.width(),
                              std::max(0, panelInner.bottom() - npcTitle.bottom() - px(6))), false);
}

void WorldProgressWidget::wheelEvent(QWheelEvent* event) {
    const int column = m_page == WorldPage::Idols || event->position().toPoint().x() < m_scrolls[1].viewport().left() ? 0 : 1;
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
