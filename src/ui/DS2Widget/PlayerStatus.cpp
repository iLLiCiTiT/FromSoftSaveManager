#include "PlayerStatus.h"

#include <QPainter>
#include <QPaintEvent>
#include <algorithm>
#include <array>

#include "Painting.h"

namespace fssm::ui::ds2 {
namespace {
    // ui_window.png is 880x543; borders that must not be stretched (same as the inventory).
    constexpr int WINDOW_W = 880;
    constexpr int WINDOW_H = 543;
    constexpr int FRAME_LEFT = 44;
    constexpr int FRAME_TOP = 66;
    constexpr int FRAME_RIGHT = 44;
    constexpr int FRAME_BOTTOM = 44;
    // Frame, fonts and rows keep one size (the look at the default window size);
    // resizing only changes how much fits.
    constexpr double FIXED_SCALE = 0.66;

    struct Attribute {
        const char* label;
        const char* icon;
        int statIndex;      // index into DS2CharacterInfo::stats (save order)
    };
    // Game's display order; the save stores adaptability last.
    constexpr std::array<Attribute, 9> ATTRIBUTES = {{
        {"VGR", "stat_vigor", 0},
        {"END", "stat_endurance", 1},
        {"VIT", "stat_vitality", 2},
        {"ATN", "stat_attunement", 3},
        {"STR", "stat_strength", 4},
        {"DEX", "stat_dexterity", 5},
        {"ADP", "stat_adaptability", 8},
        {"INT", "stat_intelligence", 6},
        {"FTH", "stat_faith", 7},
    }};

    const QColor PANEL_COLOR(10, 10, 10, 120);
    const QColor HEADER_TOP(70, 84, 96, 230);
    const QColor HEADER_BOTTOM(24, 30, 36, 230);
    const QColor LINE_COLOR(96, 86, 68);
}

PlayerStatusWidget::PlayerStatusWidget(QWidget* parent): QWidget(parent) {
    setAttribute(Qt::WA_TranslucentBackground, true);
    m_window = QPixmap(":/ds2_images/ui_window");
}

QSize PlayerStatusWidget::sizeHint() const {
    return {WINDOW_W, WINDOW_H};
}

void PlayerStatusWidget::setCharacter(const fssm::parse::ds2::DS2CharacterInfo* charInfo) {
    m_hasCharacter = charInfo != nullptr;
    if (charInfo != nullptr) m_char = *charInfo;
    update();
}

void PlayerStatusWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    m_iconCache.clear();
}

QPixmap PlayerStatusWidget::icon(const QString& name, int size) {
    const QString key = name + QString("@%1").arg(size);
    auto it = m_iconCache.find(key);
    if (it != m_iconCache.end()) return *it;
    QPixmap source(":/ds2_images/" + name);
    QPixmap result = source.isNull() ? QPixmap()
                                     : source.scaled(size, size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    m_iconCache.insert(key, result);
    return result;
}

void PlayerStatusWidget::drawRow(QPainter& painter, const QRect& rect, const QString& iconName,
                                 const QString& label, const QString& value) {
    const int iconSize = int(rect.height() * 0.82);
    const int iconTop = rect.top() + (rect.height() - iconSize) / 2;
    int textLeft = rect.left();
    if (!iconName.isEmpty()) {
        painter.drawPixmap(rect.left(), iconTop, icon(iconName, iconSize));
        textLeft += iconSize + rect.height() / 4;
    }
    const QRect text(textLeft, rect.top(), rect.right() - textLeft, rect.height());
    drawOutlinedText(painter, text, Qt::AlignLeft | Qt::AlignVCenter, label, TEXT_COLOR);
    drawOutlinedText(painter, text, Qt::AlignRight | Qt::AlignVCenter, value, TEXT_COLOR);
}

void PlayerStatusWidget::drawSectionHeader(QPainter& painter, const QRect& rect, const QString& title) {
    QLinearGradient bg(rect.topLeft(), rect.bottomLeft());
    bg.setColorAt(0.0, HEADER_TOP);
    bg.setColorAt(1.0, HEADER_BOTTOM);
    painter.setPen(QPen(LINE_COLOR, 1));
    painter.setBrush(bg);
    painter.drawRect(rect.adjusted(0, 0, -1, -1));
    drawOutlinedText(painter, rect.adjusted(rect.height() / 3, 0, 0, 0), Qt::AlignLeft | Qt::AlignVCenter,
                     title, TEXT_COLOR);
}

void PlayerStatusWidget::paintEvent(QPaintEvent* event) {
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
                     "Player Status", TEXT_COLOR);

    const QRect content = r.adjusted(int(FRAME_LEFT * s) + 4, int(FRAME_TOP * s) + 6,
                                     -(int(FRAME_RIGHT * s) + 4), -(int(FRAME_BOTTOM * s) + 2));
    if (!m_hasCharacter) {
        painter.setFont(serifFont(int(20 * s)));
        painter.setPen(DIM_TEXT_COLOR);
        painter.drawText(content, Qt::AlignCenter, "No character selected");
        return;
    }

    // Row height: 1 top row + header + 9 attribute rows, with a little air.
    const int rowH = int(34 * s);
    const int gap = std::max(4, rowH / 4);
    painter.setFont(serifFont(std::max(11, int(rowH * 0.62))));

    // Top row: level, souls, souls to next level, soul memory.
    const QRect top(content.left(), content.top(), content.width(), rowH + gap);
    painter.setPen(Qt::NoPen);
    painter.setBrush(PANEL_COLOR);
    painter.drawRect(top);
    const int cellW = top.width() / 4;
    const std::array<std::pair<const char*, QString>, 4> topCells = {{
        {"stat_level", QString::number(m_char.level)},
        {"stat_souls", QString::number(m_char.souls)},
        {"stat_next_level", QString::number(m_char.soulsToNextLevel)},
        {"stat_soul_memory", QString::number(m_char.soulMemory)},
    }};
    for (int i = 0; i < 4; ++i) {
        const QRect cell(top.left() + i * cellW + gap, top.top() + gap / 2, cellW - 2 * gap, rowH);
        drawRow(painter, cell, topCells[i].first, i == 0 ? "Lv" : "", topCells[i].second);
        if (i > 0) {
            painter.setPen(QPen(LINE_COLOR, 1));
            painter.drawLine(top.left() + i * cellW, top.top() + 3, top.left() + i * cellW, top.bottom() - 3);
        }
    }

    // Two columns: attributes and performance.
    const int colTop = top.bottom() + 1 + gap;
    const int colW = (content.width() - gap) / 2;
    const QRect attrCol(content.left(), colTop, colW, content.bottom() - colTop);
    const QRect perfCol(attrCol.right() + 1 + gap, colTop, colW, attrCol.height());
    for (const QRect& col : {attrCol, perfCol}) {
        painter.setPen(QPen(LINE_COLOR, 1));
        painter.setBrush(PANEL_COLOR);
        painter.drawRect(col.adjusted(0, 0, -1, -1));
    }
    drawSectionHeader(painter, QRect(attrCol.left(), attrCol.top(), attrCol.width(), rowH), "Attributes");
    drawSectionHeader(painter, QRect(perfCol.left(), perfCol.top(), perfCol.width(), rowH), "Performance");

    const int rowsTop = attrCol.top() + rowH + gap;
    const int rowPitch = std::min(rowH + gap / 2, (attrCol.bottom() - rowsTop) / int(ATTRIBUTES.size()));
    for (size_t i = 0; i < ATTRIBUTES.size(); ++i) {
        const Attribute& a = ATTRIBUTES[i];
        const QRect row(attrCol.left() + gap, rowsTop + int(i) * rowPitch, attrCol.width() - 2 * gap, rowH);
        drawRow(painter, row, a.icon, a.label, QString::number(m_char.stats[a.statIndex]));
    }

    const std::array<std::array<QString, 3>, 2> perf = {{
        {"stat_hp", "HP", QString::number(m_char.hp)},
        {"stat_equip_load", "Equip load", QString::number(m_char.equipLoad, 'f', 1)},
    }};
    for (size_t i = 0; i < perf.size(); ++i) {
        const QRect row(perfCol.left() + gap, rowsTop + int(i) * rowPitch, perfCol.width() - 2 * gap, rowH);
        drawRow(painter, row, perf[i][0], perf[i][1], perf[i][2]);
    }
}
}
